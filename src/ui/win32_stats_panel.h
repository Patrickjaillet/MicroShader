#pragma once

#include <cstddef>

#include "ushader/golf_core.h"

struct ID2D1RenderTarget;
struct ID2D1SolidColorBrush;
struct IDWriteFactory;
struct IDWriteTextFormat;
struct ThemeBrushes;

class Win32StatsPanel
{
public:
    bool create(ID2D1RenderTarget* render_target, IDWriteFactory* dwrite_factory);
    void destroy();

    void layout(int x, int y, int width, int height);
    // ROADMAP.md Phase 37.4 -- `original_budget` is the pre-golf source's
    // own budget estimate (raw + DEFLATE), passed alongside the golfed
    // result's `budget` so the "Golf Power" section can show a genuine
    // DEFLATE-estimated reduction percentage, not just the raw-character
    // one `stats.reduction_pct` already covers.
    // `frequency_aware_renaming_enabled` -- ROADMAP.md Phase 37.4's "Passes
    // fired" line must not infer this pass from `stats.renamed_count`
    // alone: that counter is nonzero for *any* renaming, including the
    // always-on default renamer, so without the toggle's own state the
    // dashboard would claim frequency-aware renaming fired even when it
    // was switched off and ordinary renaming did the work instead.
    void set_stats(const UshaderGolfStats& stats, std::size_t golfed_byte_size,
        const UshaderBudgetResult& budget, const UshaderBudgetResult& original_budget,
        int budget_preset_index, bool has_data, bool frequency_aware_renaming_enabled);
    void paint(ID2D1RenderTarget* render_target, const ThemeBrushes& brushes) const;

private:
    IDWriteTextFormat* text_format = nullptr;
    ID2D1SolidColorBrush* dynamic_brush = nullptr;

    UshaderGolfStats last_stats{};
    std::size_t last_golfed_bytes = 0;
    UshaderBudgetResult last_budget{};
    UshaderBudgetResult last_original_budget{};
    int last_preset_index = -1;
    bool has_data = false;
    bool last_frequency_aware_renaming_enabled = false;

    int origin_x = 0;
    int origin_y = 0;
    int width_px = 0;
    int height_px = 0;
};
