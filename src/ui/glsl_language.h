#pragma once

#include <string>

#include <TextEditor.h>

const TextEditor::LanguageDefinition& glsl_language_definition();
TextEditor::PaletteIndex classify_glsl_token(const std::string& token);
