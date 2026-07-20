#pragma once

#include <string>

#include "ushader/golf_core.h"

struct SDL_Window;

struct GolfPassToggles
{
    bool aggressive = true;
    bool eliminate_dead_locals = true;
    bool eliminate_dead_stores = true;
    bool fold_constants = true;
    bool reduce_constant_vectors = true;
    bool strip_trailing_void_return = true;
    bool compound_assignments = true;
    bool increment_decrement = true;
    bool ternary_from_if_else = true;
    bool merge_declarations = true;
    bool strip_redundant_braces = true;
    bool strip_redundant_parens = true;
    bool strip_duplicate_precision = true;
    bool eliminate_dead_functions = true;
    bool inline_single_call_functions = true;
    bool simplify_algebraic_identities = true;
    bool eliminate_common_subexpressions = true;
};

UshaderGolfOptions to_golf_options(const GolfPassToggles& toggles);
void save_golf_profile_action(const GolfPassToggles& toggles, const std::string& protected_names, int budget_preset_index, std::string& last_profile_path, SDL_Window* window);
void load_golf_profile_action(GolfPassToggles& toggles, std::string& protected_names, int& budget_preset_index, std::string& last_profile_path, SDL_Window* window);
void render_golf_controls(GolfPassToggles& toggles, std::string& protected_names, int& budget_preset_index, std::string& last_profile_path, SDL_Window* window);
