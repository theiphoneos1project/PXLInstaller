//
// Copyright (c) 2026 Nightwind
//

#ifndef APPLEFILECONDUITSESSION_H
#define APPLEFILECONDUITSESSION_H

#include <memory>
#include <vector>
#include <string>
#include <optional>
#include <string_view>
#include "usb/mux.h"

class AppleFileConduitSession {
public:
    enum class PathKind {
        NotFound = 0,
        IsFile = 1,
        IsDirectory = 2
    };

    enum class OperationType {
        Status = 0x00000001,
        ReadDirectory = 0x00000003,
        RemovePath = 0x00000008,
        MakeDirectory = 0x00000009,
        GetFileInfo = 0x0000000a,
        FileOpen = 0x0000000d,
        FileRead = 0x0000000f,
        FileWrite = 0x00000010,
        FileClose = 0x00000014,
        RenamePath = 0x00000018
    };
public:
    explicit AppleFileConduitSession(usb_pipe_t *usbPipe, uint16_t afcPort);
    bool Connect(void);

    std::optional<std::vector<std::string>> ContentsOfDirectory(std::string_view remotePath);
    bool CreateDirectory(std::string_view remotePath);
    
    std::optional<std::vector<uint8_t>> ReadFile(std::string_view remotePath);
    bool WriteFile(std::string_view remotePath, const uint8_t *const data, size_t length);
    bool DeleteFile(std::string_view remotePath);
    
    bool RenamePath(std::string_view oldPath, std::string_view newPath);
    bool PathExists(std::string_view remotePath);

    PathKind GetPathKind(std::string_view remotePath);

private:
    std::optional<std::vector<uint8_t>> Dispatch(OperationType operation, const void *payload, size_t payloadLength, size_t headerOnlyLength);
private:
    std::unique_ptr<mux_session_t> m_afcSession;
    uint64_t m_packetNumber = 0;
};

#endif // APPLEFILECONDUITSESSION_H