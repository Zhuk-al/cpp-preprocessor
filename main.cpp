#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

using namespace std;
using filesystem::path;

path operator""_p(const char* data, std::size_t sz) {
    return path(data, data + sz);
}

// напишите эту функцию
bool Preprocess(istream& input, ostream& output, const path& file_name, const vector<path>& include_directories) {
    static regex local_reg(R"/(\s*#\s*include\s*"([^"]*)"\s*)/");
    static regex general_reg(R"/(\s*#\s*include\s*<([^>]*)>\s*)/");

    string str;
    int string_counter = 0;
    while (getline(input, str)) {
        ++string_counter;
        smatch matched;
        bool outside_find = false;
        path full_path;
        // ищем файл в текущей директиве
        if (regex_match(str, matched, local_reg)) {
            path include_file = string(matched[1]);
            path current_dir = file_name.parent_path();
            full_path = current_dir / include_file;
            if (filesystem::exists(full_path)) {
                ifstream input(full_path);
                if (input.is_open()) {
                    if (!Preprocess(input, output, full_path, include_directories)) {
                        return false;
                    }
                    continue;
                }
                else {
                    cout << "unknown include file "s << full_path.filename().string()
                        << " at file " << file_name.string() << " at line "s << string_counter << endl;
                    return false;
                }
            }
            // не нашли файл в текущей директиве
            else {
                outside_find = true;
            }
        }
        // ищем файл по всем директивам
        if (outside_find || regex_match(str, matched, general_reg)) {
            bool found = false;
            for (const auto& dir : include_directories) {
                full_path = dir / string(matched[1]);
                if (filesystem::exists(full_path)) {
                    ifstream input(full_path);
                    if (input.is_open()) {
                        found = true;
                        if (!Preprocess(input, output, full_path, include_directories)) {
                            return false;
                        }
                        break;
                    }
                    else {
                        cout << "unknown include file "s << full_path.filename().string()
                            << " at file " << file_name.string() << " at line "s << string_counter << endl;
                        return false;
                    }
                }
            }
            if (!found) {
                cout << "unknown include file "s << full_path.filename().string()
                    << " at file " << file_name.string() << " at line "s << string_counter << endl;
                return false;
            }
            continue;
        }
        output << str << endl;
    }
    return true;
}

bool Preprocess(const path& in_file, const path& out_file, const vector<path>& include_directories) {
    ifstream input(in_file);
    if (!input.is_open()) {
        return false;
    }
    ofstream output(out_file);
    return Preprocess(input, output, in_file, include_directories);
}

string GetFileContents(string file) {
    ifstream stream(file);

    // конструируем string по двум итераторам
    return {(istreambuf_iterator<char>(stream)), istreambuf_iterator<char>()};
}

void Test() {
    error_code err;
    filesystem::remove_all("sources"_p, err);
    filesystem::create_directories("sources"_p / "include2"_p / "lib"_p, err);
    filesystem::create_directories("sources"_p / "include1"_p, err);
    filesystem::create_directories("sources"_p / "dir1"_p / "subdir"_p, err);

    {
        ofstream file("sources/a.cpp");
        file << "// this comment before include\n"
                "#include \"dir1/b.h\"\n"
                "// text between b.h and c.h\n"
                "#include \"dir1/d.h\"\n"
                "\n"
                "int SayHello() {\n"
                "    cout << \"hello, world!\" << endl;\n"
                "#   include<dummy.txt>\n"
                "}\n"s;
    }
    {
        ofstream file("sources/dir1/b.h");
        file << "// text from b.h before include\n"
                "#include \"subdir/c.h\"\n"
                "// text from b.h after include"s;
    }
    {
        ofstream file("sources/dir1/subdir/c.h");
        file << "// text from c.h before include\n"
                "#include <std1.h>\n"
                "// text from c.h after include\n"s;
    }
    {
        ofstream file("sources/dir1/d.h");
        file << "// text from d.h before include\n"
                "#include \"lib/std2.h\"\n"
                "// text from d.h after include\n"s;
    }
    {
        ofstream file("sources/include1/std1.h");
        file << "// std1\n"s;
    }
    {
        ofstream file("sources/include2/lib/std2.h");
        file << "// std2\n"s;
    }

    assert((!Preprocess("sources"_p / "a.cpp"_p, "sources"_p / "a.in"_p,
                                  {"sources"_p / "include1"_p,"sources"_p / "include2"_p})));

    ostringstream test_out;
    test_out << "// this comment before include\n"
                "// text from b.h before include\n"
                "// text from c.h before include\n"
                "// std1\n"
                "// text from c.h after include\n"
                "// text from b.h after include\n"
                "// text between b.h and c.h\n"
                "// text from d.h before include\n"
                "// std2\n"
                "// text from d.h after include\n"
                "\n"
                "int SayHello() {\n"
                "    cout << \"hello, world!\" << endl;\n"s;

    assert(GetFileContents("sources/a.in"s) == test_out.str());
}

int main() {
    Test();
}