// Multi-provider AI domain type — analogue of the Swift sibling's
// `AIProvider` but tuned for OpenAI-compatible HTTP backends (Groq,
// OpenAI, OpenRouter, Together, any custom base URL).
//
// The Swift sibling's `AIProvider` describes CLI launchers (claude / gemini
// / codex) — that lives in a separate surface. This one models the *chat*
// + *whisper* services driven by the existing GroqClient.
#pragma once

#include <QString>

namespace dante::ai {

struct AIProvider {
    QString id;          // stable UUID
    QString name;        // user-facing label
    QString baseUrl;     // e.g. "https://api.openai.com/v1"
    // Logical key name for future secret-storage backends. For MVP we
    // simply stash the literal API key here (matching Swift's plaintext
    // posture in credentials.json), so callers can read the secret
    // directly from this field without an extra lookup step.
    QString apiKeyRef;
    QString model;       // default model id for this provider
    QString kind;        // "chat" | "whisper" | "both"
    bool    enabled{true};
};

} // namespace dante::ai
