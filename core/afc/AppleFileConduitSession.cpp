#include "AppleFileConduitSession.hpp"
#include <iostream>
#include <random>

#define AFC_MAGIC "CFA6LPAA"

#pragma pack(push, 1)
typedef struct {
    char magic[8];
    uint64_t entire_length;
    uint64_t this_length;
    uint64_t packet_num;
    uint64_t operation;
} afc_header_t;
#pragma pack(pop)

static uint16_t RandomPort(void) {
    static std::mt19937 random(std::random_device{}());
    static std::uniform_int_distribution<uint16_t> distribution(49152, 65535);
    return distribution(random);
}

AppleFileConduitSession::AppleFileConduitSession(usb_pipe_t *usbPipe, uint16_t afcPort) : m_afcSession(std::make_unique<mux_session_t>()) {
    m_afcSession->pipe = usbPipe;
    m_afcSession->device_port = afcPort;
    m_afcSession->src_port = static_cast<uint16_t>(RandomPort());
    m_afcSession->tx_seq = 100;
    m_afcSession->rx_ack = 0;
}

bool AppleFileConduitSession::Connect(void) {
    return session_connect(m_afcSession.get()) == 0;
}

std::optional<std::vector<std::string>> AppleFileConduitSession::ContentsOfDirectory(std::string_view remotePath) {
    auto response = Dispatch(OperationType::ReadDirectory, remotePath.data(), remotePath.length() + 1, remotePath.length() + 1);
    if (!response) {
        return std::nullopt;
    }
    
    std::vector<std::string> entries;

    const char *pointer = reinterpret_cast<const char *>(response->data());
    const char *end = pointer + response->size();

    while (pointer < end && *pointer != '\0') {
        entries.emplace_back(pointer);
        pointer += strlen(pointer) + 1;
    }

    return entries;
}

bool AppleFileConduitSession::CreateDirectory(std::string_view remotePath) {
    auto response = Dispatch(OperationType::MakeDirectory, remotePath.data(), remotePath.size() + 1, remotePath.size() + 1);

    if (!response.has_value()) {
        std::cerr << "[AppleFileConduitSession] Make directory failed\n";
        return false;
    }
    
    return true;
}

std::optional<std::vector<uint8_t>> AppleFileConduitSession::ReadFile(std::string_view remotePath) {
    const size_t remotePathLength = remotePath.size() + 1;
    
    uint8_t openPayload[8 + 512];
    if (remotePathLength > sizeof(openPayload) - 8) {
        std::cerr << "[AppleFileConduitSession] Path too long\n";
        return std::nullopt;
    }

    uint64_t mode = static_cast<uint64_t>(OperationType::Status);
    memcpy(openPayload, &mode, 8);
    memcpy(openPayload + 8, remotePath.data(), remotePathLength);

    auto response = Dispatch(OperationType::FileOpen, openPayload, 8 + remotePathLength, 8 + remotePathLength);
    if (!response.has_value()) {
        std::cerr << "[AppleFileConduitSession] File open failed\n";
        return std::nullopt;
    }

    uint64_t handle;
    memcpy(&handle, response->data(), 8);

    std::vector<uint8_t> contents;
    while (true) {
        uint8_t readRequest[16];
        uint64_t wantLength = 4096;
            
        memcpy(readRequest, &handle, sizeof(handle));
        memcpy(readRequest + 8, &wantLength, sizeof(wantLength));

        auto chunk = Dispatch(OperationType::FileRead, readRequest, 16, 16);
        if (!chunk) {
            Dispatch(OperationType::FileClose, &handle, 8, 8);
            return std::nullopt;
        }
        
        if (chunk->empty()) {
            break;
        }
        
        contents.insert(contents.end(), chunk->begin(), chunk->end());
        
        if (chunk->size() < wantLength) {
            break;
        }
    }

    Dispatch(OperationType::FileClose, &handle, 8, 8);
    return contents;
}

bool AppleFileConduitSession::WriteFile(std::string_view remotePath, const uint8_t *const data, size_t length) {
    const size_t remotePathLength = remotePath.size() + 1;
    
    uint8_t openPayload[8 + 512];
    if (remotePathLength > sizeof(openPayload) - 8) {
        std::cerr << "[AppleFileConduitSession] Path too long\n";
        return false;
    }

    uint64_t mode = static_cast<uint64_t>(OperationType::ReadDirectory);
    memcpy(openPayload, &mode, 8);
    memcpy(openPayload + 8, remotePath.data(), remotePathLength);

    auto response = Dispatch(OperationType::FileOpen, openPayload, 8 + remotePathLength, 8 + remotePathLength);
    if (!response.has_value()) {
        std::cerr << "[AppleFileConduitSession] File open failed\n";
        return false;
    }

    if (response->size() < 8) {
        std::cerr << "[AppleFileConduitSession] Open response missing handle\n";
        return false;
    }

    uint64_t handle;
    memcpy(&handle, response->data(), 8);

    size_t sentBytes = 0;
    while (sentBytes < length) {
        size_t chunkSize = std::min<size_t>(length - sentBytes, 4096);
        
        uint8_t writePayload[8 + 4096];
        memcpy(writePayload, &handle, 8);
        memcpy(writePayload + 8, data + sentBytes, chunkSize);

        if (!Dispatch(OperationType::FileWrite, writePayload, 8 + chunkSize, 8)) {
            std::cerr << "[AppleFileConduitSession] File write failed\n";
            Dispatch(OperationType::FileClose, &handle, 8, 8);
            return false;
        }
        
        sentBytes += chunkSize;
    }

    Dispatch(OperationType::FileClose, &handle, 8, 8);
    return true;
}

bool AppleFileConduitSession::DeleteFile(std::string_view remotePath) {
    auto response = Dispatch(OperationType::RemovePath, remotePath.data(), remotePath.size() + 1, remotePath.size() + 1);

    if (!response.has_value()) {
        std::cerr << "[AppleFileConduitSession] Remove path failed\n";
        return false;
    }
    
    return true;
}

bool AppleFileConduitSession::RenamePath(std::string_view oldPath, std::string_view newPath) {
    const size_t oldLength = oldPath.size() + 1;
    const size_t newLength = newPath.size() + 1;

    uint8_t payload[512 + 512];
    if (oldLength + newLength > sizeof(payload)) {
        std::cerr << "[AppleFileConduitSession] Rename paths too long\n";
        return false;
    }

    memcpy(payload, oldPath.data(), oldLength);
    memcpy(payload + oldLength, newPath.data(), newLength);

    return Dispatch(OperationType::RenamePath, payload, oldLength + newLength, oldLength + newLength).has_value();
}

bool AppleFileConduitSession::PathExists(std::string_view remotePath) {
    switch (GetPathKind(remotePath)) {
        case PathKind::NotFound: return false;
        case PathKind::IsFile: return true;
        case PathKind::IsDirectory: return true;
    }

    return false;
}

AppleFileConduitSession::PathKind AppleFileConduitSession::GetPathKind(std::string_view remotePath) {
    auto response = Dispatch(OperationType::GetFileInfo, remotePath.data(), remotePath.size() + 1, remotePath.size() + 1);

    if (!response.has_value()) {
        return PathKind::NotFound;
    }

    const uint8_t *data = response->data();
    size_t responseLength = response->size();
    
    for (const char *pointer = reinterpret_cast<const char *>(data); pointer < reinterpret_cast<const char *>(data) + responseLength;) {
        size_t keyLength = strlen(pointer);
        const char *key = pointer;
        pointer += keyLength + 1;
        if (pointer >= reinterpret_cast<const char *>(data) + responseLength) {
            break;
        }

        size_t valueLength = strlen(pointer);
        const char *value = pointer;
        pointer += valueLength + 1;

        if (strcmp(key, "st_ifmt") == 0) {
            if (strcmp(value, "S_IFDIR") == 0) {
                return PathKind::IsDirectory;
            }
            return PathKind::IsFile;
        }
    }

    return PathKind::IsFile;
}

std::optional<std::vector<uint8_t>> AppleFileConduitSession::Dispatch(OperationType operation, const void *payload, size_t payloadLength, size_t headerOnlyLength) {
    uint8_t buffer[sizeof(afc_header_t) + 8192];
    if (payloadLength > (sizeof(buffer) - sizeof(afc_header_t))) {
        std::cerr << "[AppleFileConduitSession] Payload too large: " << payloadLength << '\n';
        return std::nullopt;
    }

    afc_header_t *header = reinterpret_cast<afc_header_t *>(buffer);
    memcpy(header->magic, AFC_MAGIC, 8);
    header->entire_length = sizeof(afc_header_t) + payloadLength;
    header->this_length = sizeof(afc_header_t) + headerOnlyLength;
    header->packet_num = m_packetNumber;
    header->operation = static_cast<uint64_t>(operation);

    m_packetNumber += 1;

    if (payloadLength > 0) {
        memcpy(buffer + sizeof(afc_header_t), payload, payloadLength);
    }

    if (session_send_frame(m_afcSession.get(), TCP_ACK, buffer, static_cast<int>(sizeof(afc_header_t) + payloadLength)) < 0) {
        std::cerr << "[AppleFileConduitSession] Send failed\n";
        return std::nullopt;
    }

    uint8_t responseBuffer[sizeof(afc_header_t) + 8192];
    size_t bytesReceived = 0;
    
    while (bytesReceived < sizeof(afc_header_t)) {
        int newBytes = session_recv(m_afcSession.get(), responseBuffer + bytesReceived, static_cast<int>(sizeof(responseBuffer) - bytesReceived));
        
        if (newBytes < 0) {
            std::cerr << "[AppleFileConduitSession] Failed receiving header\n";
            return std::nullopt;
        }
        
        bytesReceived += static_cast<size_t>(newBytes);
    }
    
    afc_header_t *responseHeader = reinterpret_cast<afc_header_t *>(responseBuffer);
    if (memcmp(responseHeader->magic, AFC_MAGIC, 8) != 0) {
        std::cerr << "[AppleFileConduitSession] Bad magic in response\n";
        return std::nullopt;
    }

    if (responseHeader->entire_length < sizeof(afc_header_t)) {
        std::cerr << "[AppleFileConduitSession] Impossibly short entire_length\n";
        return std::nullopt;
    }
    
    if (responseHeader->entire_length > sizeof(responseBuffer)) {
        std::cerr << "[AppleFileConduitSession] Response too large: " << responseHeader->entire_length << '\n';
        return std::nullopt;
    }

    while (bytesReceived < responseHeader->entire_length) {
        int newBytes = session_recv(m_afcSession.get(), responseBuffer + bytesReceived, static_cast<int>(sizeof(responseBuffer) - bytesReceived));
        
        if (newBytes <= 0) {
            std::cerr << "[AppleFileConduitSession] Failed receiving packet\n";
            return std::nullopt;
        }

        bytesReceived += static_cast<size_t>(newBytes);
    }

    size_t payloadSize = bytesReceived - sizeof(afc_header_t);
    const uint8_t *payloadStart = responseBuffer + sizeof(afc_header_t);
    
    if (static_cast<OperationType>(responseHeader->operation) == OperationType::Status && payloadSize >= sizeof(uint64_t)) {
        uint64_t status;
        memcpy(&status, payloadStart, 8);
        if (status != 0) {
            std::cerr << "[AppleFileConduitSession] AFC status error: " << status << '\n';
            return std::nullopt;
        }
    }
    
    return std::vector<uint8_t>(payloadStart, payloadStart + payloadSize);
}
