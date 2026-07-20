#pragma once

#include <memory>
#include <string>
#include <vector>

#include <TextEditor.h>

#include "diff_view.h"
#include "golf_controls.h"
#include "golf_trace.h"
#include "../render/shader_runner.h"
#include "ushader/golf_core.h"

struct Document
{
    int tab_id = 0;
    std::string file_path;
    std::string display_name = "untitled";
    std::string saved_source;
    bool dirty = false;

    TextEditor source_editor;
    TextEditor golfed_editor;

    std::string golfed_text;
    std::string compile_error;
    bool formatted_view = false;

    GolfPassToggles pass_toggles;
    std::string protected_names;
    int budget_preset_index = 3;

    UshaderGolfStats golf_stats{};
    UshaderBudgetResult budget_result{};
    std::vector<GolfTraceStep> golf_trace;
    std::vector<DiffSpan> diff_spans;

    EquivalenceRunResult equivalence_result;
};

std::string document_display_name(const std::string& file_path);
void configure_document_editors(Document& document);
std::unique_ptr<Document> make_document(const std::string& source_text, const std::string& file_path);
std::unique_ptr<Document> make_default_document();
