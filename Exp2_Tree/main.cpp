#include "compressor.hpp"
#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <algorithm>

using namespace chr;

void print_help() {
    std::cout << "========== Huffman压缩工具命令行模式 ==========\n";
    std::cout << "命令格式: -command [参数]\n";
    std::cout << "可用命令:\n";
    std::cout << "  -cmp -src <path> [-dir <path>] [-name <name>] [-o <option>]  压缩文件\n";
    std::cout << "  -dmp -src <path> [-dir <path>] [-name <name>] [-o <option>]  解压文件\n";
    std::cout << "  -clear                                                        清空屏幕\n";
    std::cout << "  -exit                                                         退出程序\n";
    std::cout << "  -help                                                         显示帮助\n";
    std::cout << "选项说明:\n";
    std::cout << "  -o 1: 显示压缩率\n";
    std::cout << "  -o 2: 显示Huffman树\n";
    std::cout << "  -o 3: 显示全部信息\n";
    std::cout << "示例:\n";
    std::cout << "  -cmp -src \"test.txt\" -o 3\n";
    std::cout << "  -dmp -src \"test.txt.huff\" -dir \"output\" -name \"decompressed.txt\"\n";
}

bool parse_command(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "错误: 缺少命令参数\n";
        print_help();
        return false;
    }

    std::string command = argv[1];

    if (command == "-help") {
        print_help();
        return true;
    }
    else if (command == "-clear") {
        system("cls");
        return true;
    }
    else if (command == "-exit") {
        std::cout << "感谢使用，再见!\n";
        return false;
    }
    else if (command == "-cmp" || command == "-dmp") {
        bool is_decompress = (command == "-dmp");

        // 解析参数
        std::string src_path, dir_path, name;
        int option = 0;

        for (int i = 2; i < argc; i++) {
            std::string arg = argv[i];

            if (arg == "-src" && i + 1 < argc) {
                src_path = argv[++i];
            }
            else if (arg == "-dir" && i + 1 < argc) {
                dir_path = argv[++i];
            }
            else if (arg == "-name" && i + 1 < argc) {
                name = argv[++i];
            }
            else if (arg == "-o" && i + 1 < argc) {
                option = std::stoi(argv[++i]);
                if (option < 1 || option > 3) {
                    std::cout << "错误: -o 参数必须是 1, 2 或 3\n";
                    return false;
                }
            }
            else {
                std::cout << "错误: 未知参数或缺少参数值: " << arg << "\n";
                return false;
            }
        }

        // 验证必要参数
        if (src_path.empty()) {
            std::cout << "错误: 必须使用 -src 指定源文件路径\n";
            return false;
        }

        // 构建目标路径
        std::filesystem::path src(src_path);
        std::filesystem::path dst_dir = dir_path.empty() ? src.parent_path() : std::filesystem::path(dir_path);
        std::filesystem::path dst_name;

        if (name.empty()) {
            if (is_decompress) {
                if (src.extension() != ".huff") {
                    std::cout << "错误: 解压文件必须是 .huff 格式\n";
                    return false;
                }
                dst_name = src.stem(); // 去掉 .huff 后缀
            }
            else {
                dst_name = src.filename().string() + ".huff";
            }
        }
        else {
            dst_name = name;
        }

        std::filesystem::path dst_path = dst_dir / dst_name;

        // 执行压缩或解压
        bool show_rate = (option & 1) != 0;
        bool show_tree = (option & 2) != 0;

        try {
            if (is_decompress) {
                decompress(src_path, dst_path.string(), show_rate, show_tree);
            }
            else {
                compress(src_path, dst_path.string(), show_rate, show_tree);
            }
            std::cout << "操作完成: " << dst_path.string() << "\n";
        }
        catch (const std::exception& e) {
            std::cout << "操作失败: " << e.what() << std::endl;
            return false;
        }
    }
    else {
        std::cout << "错误: 未知命令: " << command << "\n";
        print_help();
        return false;
    }

    return true;
}

int main(int argc, char* argv[]) {
    if (argc > 1) {
        // 命令行模式
        return parse_command(argc, argv) ? 0 : 1;
    }

    // 交互模式
    std::cout << "欢迎使用Huffman压缩工具!\n";
    std::cout << "输入 -help 查看可用命令\n";

    std::string input;
    while (true) {
        std::cout << "\n> ";
        std::getline(std::cin, input);

        if (input.empty()) continue;

        // 解析输入为命令行参数
        std::vector<std::string> args;
        std::istringstream iss(input);
        std::string token;

        while (iss >> token) {
            // 处理引号包围的参数
            if (token.front() == '"') {
                std::string quoted_arg = token;
                while (quoted_arg.back() != '"' && iss >> token) {
                    quoted_arg += " " + token;
                }
                if (quoted_arg.back() == '"') {
                    args.push_back(quoted_arg.substr(1, quoted_arg.length() - 2));
                }
                else {
                    args.push_back(quoted_arg.substr(1));
                }
            }
            else {
                args.push_back(token);
            }
        }

        if (args.empty()) continue;

        // 转换为char*数组格式
        std::vector<char*> cargs;
        cargs.push_back(argv[0]); // 程序名
        for (auto& arg : args) {
            cargs.push_back(&arg[0]);
        }

        if (!parse_command(cargs.size(), cargs.data())) {
            if (args[0] == "-exit") {
                break;
            }
        }
    }

    std::cout << "感谢使用，再见!\n";
    return 0;
}