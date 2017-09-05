#include <cstdio>
#include <string>
#include <unistd.h>
#include <iostream>
#include <locale>    // for wstring_convert
#include <codecvt>   // for codecvt_utf8

#include "config.h"

#include "cmake_include.h"
#ifdef ENABLE_GIT
#include <git2.h>
#endif


int last_return_value;
std::string current_bg;

std::string primary_fg;


#ifndef NO_POWERLINE
#   define SEGMENT_SEPARATOR "\ue0b0"
#   define PLUSMINUS "\u00b1"
#   define BRANCH "\ue0a0"
#   define DETACHED "\u27a6"
#   define CROSS "\u2718"
#   define LIGHTNING "\u26a1"
#   define GEAR "\u2699"
#else
#   define SEGMENT_SEPARATOR "▒"
#   define PLUSMINUS "[+-]"
#   define BRANCH "╫"
#   define DETACHED "[?]"
#   define CROSS "╳"
#   define LIGHTNING "(root)"
#   define GEAR "[j]"
#endif

void prompt_segment(std::string const& bg, std::string const& fg) {
    std::string theBG, theFG;
    if(bg != "%k") theBG = "%K{" + bg + "}";
    else theBG = bg;
    if(fg != "%f") theFG = "%F{" + fg + "}";
    else theFG = fg;

    if(current_bg.empty() || bg != current_bg) {
        printf("%%{%s%%F{%s}%%}" SEGMENT_SEPARATOR "%%{%s%%}", theBG.c_str(), current_bg.c_str(), theFG.c_str());
    } else {
        printf("%%{%s%%}%%{%s%%}", theBG.c_str(), theFG.c_str());
    }

    current_bg = bg;
}

void prompt_segment(std::string const& bg, std::string const& fg, std::string const& r) {
    prompt_segment(bg, fg);
    printf("%s", r.c_str());
}


void prompt_end() {
    if(current_bg.empty()) {
        printf("%%{%%k%%F{red}%%}" SEGMENT_SEPARATOR);
    } else {
        printf("%%{%%k%%F{%s}%%}" SEGMENT_SEPARATOR, current_bg.c_str());
    }

    printf("%%{%%f%%}");
    current_bg = "";
}


void prompt_context() {
    std::string user;
    if(getlogin()) user = getlogin();
    else if(getenv("USER")) user = getenv("USER");

    if(!user.empty()) {

        const char *default_user = getenv("DEFAULT_USER");
        if ((!(default_user && user == default_user) || !getenv("SSH_CONNECTION"))) {
            prompt_segment(primary_fg, "default", std::string(" %(!.%{%F{yellow}%}.)") + user + "@%m ");
        }
    }
}


void prompt_git() {
#ifdef ENABLE_GIT

    git_libgit2_init();

    auto buf_size = static_cast<size_t>(pathconf(".", _PC_PATH_MAX));
    auto * buf = (char*)malloc(sizeof(char) * buf_size);
    buf = getcwd(buf, buf_size);

    git_repository* repo;
    int error = git_repository_open_ext(&repo, buf, GIT_REPOSITORY_OPEN_CROSS_FS, nullptr);

    free(buf);


    if(error == GIT_OK) {
        std::string color;

        git_reference* reference;
        git_repository_head(&reference, repo);

        std::string ref = git_reference_name(reference);
        bool is_dirty = false;

#ifdef ENABLE_GIT_DIRTY_CHECK_VIA_LIBGIT2

        {
            git_status_list *status;
            git_status_options opt = {0};
            opt.show = GIT_STATUS_SHOW_INDEX_AND_WORKDIR;
            opt.flags = GIT_STATUS_OPT_DEFAULTS | GIT_STATUS_OPT_EXCLUDE_SUBMODULES;

            if (git_status_list_new(&status, repo, &opt)) {

                auto entries = git_status_list_entrycount(status);

                printf("%d\n", entries);
         //       is_dirty = entries > 0;
                for (size_t i = 0; i < entries; ++i) {
                    auto entry = git_status_byindex(status, i);
                    if (entry->status != GIT_STATUS_CURRENT) {
                        is_dirty = true;
                        break;
                    }
                }

                git_status_list_free(status);
            }
        }
#else
    is_dirty = system("test -n \"$(git status --porcelain --ignore-submodules)\"") == 0;
#endif

        if (!ref.empty()) {
            std::string prefix = "refs/heads/";
            if(ref.substr(0, prefix.size()) == prefix)
                ref = ref.substr(prefix.size());

            if (is_dirty) {
                color = "yellow";
                ref = ref + " " PLUSMINUS;
            } else {
                color = "blue";
                ref = ref + " ";
            }
        }

        if(git_repository_head_detached(repo)) {
            ref = std::string(DETACHED) + " " + ref;
        } else {
            ref = std::string(BRANCH) + " " + ref;
        }

        prompt_segment(color, primary_fg);
        printf(" %s", ref.c_str());

        git_repository_free(repo);
    }


    git_libgit2_shutdown();
#endif
}


void prompt_dir() {
    prompt_segment("green", primary_fg, " %~ ");
}


std::string get_filesystem(std::string const& folder) {
    std::string fs;
    FILE * stream;
    const int max_buffer = 256;
    char buffer[max_buffer];

    stream = popen((std::string("df -PTh ") + folder + " | awk '{print $2}' | tail -1").c_str(), "r");
    if (stream) {
        while (!feof(stream))
            if (fgets(buffer, max_buffer, stream) != NULL) fs.append(buffer);
        pclose(stream);
    }

    return fs.substr(0, fs.length() - 1);
}

void prompt_filesystem() {

    auto fs = get_filesystem("$(pwd)");
    auto default_fs = get_filesystem("/");

    if(fs != default_fs) {
        prompt_segment("magenta", primary_fg, " " + fs + " ");
    }
}


void prompt_status() {
    std::u16string symbols;

    if(last_return_value != 0) symbols += u"%{%F{red}%}" CROSS;
    if(getuid() == 0) symbols+=u"%{%F{yellow}%}" LIGHTNING;
    if(system("[[ $(jobs -l | wc -l) -gt 0 ]]") == 0) symbols+=u"%{%F{cyan}%}" GEAR;

    if(!symbols.empty()) {
        std::wstring_convert<std::codecvt_utf8<char16_t>, char16_t> cv;

        prompt_segment(primary_fg, "default", cv.to_bytes(symbols.c_str()) + " ");

    }
}


void prompt_virtualenv() {
    if(getenv("VIRTUAL_ENV")) {
        prompt_segment("cyan", primary_fg);
        printf(" %s ", getenv("VIRTUAL_ENV"));
    }
}


int main(int argc, const char** argv) {
    if(argc < 2)
        return 1;

    last_return_value = atoi(argv[1]);

    current_bg = "";

    if(getenv("PRIMARY_FG")) {
        primary_fg = getenv("PRIMARY_FG");
    } else {
        primary_fg = "black";
    }


    prompt_status();
    prompt_context();
    prompt_dir();

    prompt_filesystem();

#ifdef ENABLE_GIT
    prompt_git();
#endif

#ifdef ENABLE_VIRTUAL_ENV
    prompt_virtualenv();
#endif

    prompt_end();

    return 0;
}