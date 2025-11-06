#include "compressor.hpp"

int main() {
    std::string str;
    while (1) {
        try {
            std::cout << "输入文本：";
            std::getline(std::cin, str);
            if (str == "exit") {
                return 0;
            }
            else if (str == "clear") {
                system("cls");
            }
            else {
                chr::huffman_tree<char> comp(str.begin(), str.end());
                std::cout << "哈夫曼码：" << "\n";
                std::cout << comp.to_string();
                auto encoded = comp.encode(str.begin(), str.end());
                std::cout << "编码结果：" << encoded.to_string(1) << "\n";
                auto decoded = comp.decode(encoded);
                std::cout << "解码结果：";
                for (auto k : decoded) {
                    std::cout << k;
                }
                std::cout << "\n";
                comp.print_compression_info(str.begin(), str.end());
            }
        }
        catch (std::runtime_error& e) {
            std::cout << e.what() << std::endl;
        }
    }
    return 0;
}