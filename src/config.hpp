// Loads the exit dialog's UI strings from crosshair.yml, falling back to
// built-in English strings and auto-generating a default file when none
// exists. There is no per-language selection: whatever text is in
// crosshair.yml is used as-is, regardless of the system language.
#pragma once

#include <string>

struct DialogStrings {
    std::string title;
    std::string info;
    std::string exit_button;
    std::string cancel_button;
};

class Config {
public:
    // Loads strings from <base_dir>/crosshair.yml. If the file does not
    // exist, it is created with the built-in English defaults, which also
    // serve as the in-memory fallback for this run.
    explicit Config(const std::string& base_dir);

    // Returns the exit dialog strings loaded from crosshair.yml (or the
    // built-in English fallback if the file is missing/invalid).
    DialogStrings get_dialog_strings() const { return strings_; }

private:
    std::string yml_path_;
    DialogStrings strings_;

    void load_or_create();
    static void write_default_file(const std::string& path, const DialogStrings& defaults);
};
