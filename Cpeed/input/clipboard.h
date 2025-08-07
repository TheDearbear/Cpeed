#pragma once

typedef enum CpdClipboardContentType {
    CpdClipboardContentType_Invalid,
    CpdClipboardContentType_Text
} CpdClipboardContentType;

typedef enum CpdClipboardActionType {
    CpdClipboardActionType_Cut,
    CpdClipboardActionType_Copy,
    CpdClipboardActionType_Paste
} CpdClipboardActionType;

typedef struct CpdClipboardInputEventData {
    CpdClipboardActionType action_type;
} CpdClipboardInputEventData;
