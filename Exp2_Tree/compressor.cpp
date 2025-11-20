#include "compressor.hpp"

namespace chr {
	// 计算节点深度（叶节点深度为 0）
	unsigned huffman_node::depth() const {
		if (this->is_leaf()) {
			return 0;
		}
		// 递归取子树最大深度 + 1
		return std::max(left->depth(), right->depth()) + 1;
	}

	// 用于优先队列的比较器：
	// 1. 频率越小优先级越高（返回 true 表示 a 的优先级低于 b）
	// 2. 若频率相同，深度更小的优先（构造更紧凑的树）
	// 3. 最后按数据字节值作稳定性比较
	bool huffman_node_comparator::operator()(const std::shared_ptr<huffman_node>& a, const std::shared_ptr<huffman_node>& b) {
		if (a->frequency != b->frequency) {
			return a->frequency > b->frequency;
		}
		if (a->depth() != b->depth()) {
			return a->depth() > b->depth();
		}
		// 这里使用 data 保持比较的确定性（避免相等频率/深度时的不确定性）
		return a->data > b->data;
	}

	// 将单个位追加到位数组末尾
	void byte_array::push_back(bool bit) {
		size_t byte_index = m_bit_count / 8;
		size_t bit_offset = m_bit_count % 8;
		// 如果超出当前字节容器，则扩容
		while (byte_index >= m_data.size()) {
			m_data.push_back(0);
		}
		// 高位优先存放：第 0 位位于字节的最高位 (1 << 7)
		if (bit) {
			m_data[byte_index] |= (1 << (7 - bit_offset));
		}
		++m_bit_count;
	}

	// 从末尾移除一个位（会将对应位清零）
	void byte_array::pop_back() {
		if (m_bit_count == 0) {
			throw std::out_of_range("弹出元素时数组为空");
		}
		--m_bit_count;
		size_t byte_index = m_bit_count / 8;
		size_t bit_offset = m_bit_count % 8;
		// 将该位清零
		m_data[byte_index] &= ~(1 << (7 - bit_offset));
		// 如果刚好移除的是字节的最后一个位且该字节已不需要，则删除该字节
		if (bit_offset == 0 && !m_data.empty()) {
			m_data.pop_back();
		}
	}

	// 将另外一个位数组逐位追加到当前数组末尾（按位复制）
	byte_array& byte_array::operator+=(const byte_array& other) {
		for (size_t i = 0; i < other.m_bit_count; ++i) {
			this->push_back(other.bit(i));
		}
		return *this;
	}

	// 读取指定位置的位（按位索引，从 0 开始）
	bool byte_array::bit(size_t pos) const {
		if (pos >= m_bit_count) {
			throw std::out_of_range("下标出界");
		}
		size_t byte_index = pos / 8;
		size_t bit_offset = pos % 8;
		// 返回对应位（高位优先）
		return (m_data[byte_index] >> (7 - bit_offset)) & 1;
	}

	// 设置指定位置的位为 0 或 1
	void byte_array::set_bit(size_t pos, bool bit) {
		if (pos >= m_bit_count) {
			throw std::out_of_range("下标出界");
		}
		size_t byte_index = pos / 8;
		size_t bit_offset = pos % 8;
		if (bit) {
			m_data[byte_index] |= (1 << (7 - bit_offset));
		}
		else {
			m_data[byte_index] &= ~(1 << (7 - bit_offset));
		}
	}

	// 将位数组转换为字符串，支持按二进制输出或按字节的十六进制输出（用于调试/显示）
	std::string byte_array::to_string(bool in_hexadecimal) const {
		std::ostringstream oss;
		if (in_hexadecimal) {
			// 按字节以两位十六进制输出（便于查看原始字节内容）
			for (byte b : m_data) {
				oss << std::hex << std::setw(2) << std::setfill('0')
					<< static_cast<int>(b) << " ";
			}
		}
		else {
			// 按位输出 0/1，每 8 位用空格分隔显示更清晰
			for (size_t i = 0; i < m_bit_count; ++i) {
				oss << this->bit(i) ? '1' : '0';
				if ((i + 1) % 8 == 0 && i + 1 < m_bit_count) {
					oss << " ";
				}
			}
		}
		return oss.str();
	}

	// 将单字节转换为可读字符串：可打印字符直接显示，否则以十六进制显示
	std::string to_string(byte data) {
		std::ostringstream oss;
		if (data >= ' ' && data <= '~') {
			oss << static_cast<char>(data);
		}
		else {
			oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(data);
		}
		return oss.str();
	}

	// 比较两个位数组是否相等（按位比较，注意处理未满字节的尾部位）
	bool operator==(const byte_array& a, const byte_array& b) {
		if (a.size() != b.size()) {
			return 0;
		}
		// 比较完整的字节部分
		size_t full_bytes = a.size() / 8;
		for (size_t i = 0; i < full_bytes; ++i) {
			if (a.data()[i] != b.data()[i]) {
				return 0;
			}
		}
		// 处理剩余的不足一个字节的部分（若存在）
		size_t remaining_bits = a.size() / 8;
		if (remaining_bits > 0) {
			// 构造掩码以只比较有效的高位部分
			byte mask = (0xFF << (8 - remaining_bits));
			if ((a.data()[full_bytes] & mask) != (b.data()[full_bytes] & mask)) {
				return 0;
			}
		}
		return 1;
	}

	// 将文件压缩为 .huff 文件，文件头包含序列化的 Huffman 树和编码的数据
	void compress(const std::filesystem::path& src_path, const std::filesystem::path& dst_path, bool show_rate, bool show_tree) {
		// 常见压缩/容器/多媒体文件后缀列表，用于避免重复压缩这类文件
		static std::vector<std::string> postfixs = {
			".zip",".rar",".7z",".gz",".tar",
			".jpg",".jpeg",".png",".gif",".bmp",
			".mp3",".mp4",".avi","mkv",
			".pdf",".docx",".xlsx",".pptx"
		};
		for (const auto& str : postfixs) {
			if (src_path.string().ends_with(str)) {
				throw std::runtime_error("文件类型已经是压缩格式，不建议再次压缩：" + src_path.string());
			}
		}
		std::ifstream ifs(src_path, std::ios::binary);
		if (!ifs.is_open()) {
			throw std::runtime_error("文件打开失败：" + src_path.string());
		}
		// 读取整个文件到字节向量
		std::vector<byte> file_data;
		file_data.assign(
			std::istreambuf_iterator<char>(ifs),
			std::istreambuf_iterator<char>()
		);
		ifs.close();

		// 构造 Huffman 树
		huffman_tree tree(file_data);
		if (show_tree) {
			tree.print_as_tree(1);
		}

		// 将树结构序列化为位数组，以便写入文件头
		byte_array tree_structure = tree.to_byte_array();

		std::ofstream ofs(dst_path, std::ios::binary);
		if (!ofs.is_open()) {
			throw std::runtime_error("无法创建压缩文件：" + dst_path.string());
		}

		// 写入树的位数和字节数（便于解压时重建位数组）
		std::size_t tree_bit_count = tree_structure.size();
		std::size_t tree_byte_count = tree_structure.byte_size();
		ofs.write(reinterpret_cast<const char*>(&tree_bit_count), sizeof(tree_bit_count));
		ofs.write(reinterpret_cast<const char*>(&tree_byte_count), sizeof(tree_byte_count));
		// 写入树序列化的原始字节数据
		ofs.write(reinterpret_cast<const char*>(tree_structure.data().data()), tree_structure.data().size());

		// 编码原始数据并写入编码信息和编码后的字节流
		auto compressed_pair = tree.encode_with_info(file_data);
		auto& compressed = compressed_pair.first;
		if (show_rate) {
			std::cout << compressed_pair.second;
		}
		const auto& compressed_data = compressed.data();
		size_t bit_count = compressed.size();
		size_t byte_count = compressed_data.size();
		ofs.write(reinterpret_cast<const char*>(&bit_count), sizeof(bit_count));
		ofs.write(reinterpret_cast<const char*>(&byte_count), sizeof(byte_count));
		ofs.write(reinterpret_cast<const char*>(compressed_data.data()), byte_count);
		ofs.close();

		// 输出压缩率信息
		if (show_rate) {
			auto src_size = std::filesystem::file_size(src_path);
			auto dst_size = std::filesystem::file_size(dst_path);
			double compression_ratio = (1 - (double)dst_size / src_size) * 100;
			std::ostringstream oss;
			oss << "原始文件大小：" << src_size / 1024.0 << " KB\n";
			oss << "压缩文件大小：" << dst_size / 1024.0 << " KB\n";
			oss << "实际压缩率：" << std::fixed << std::setprecision(2) << compression_ratio << "%\n";
			std::cout << oss.str();
		}
	}

	// 解压 .huff 文件：读取头部的树结构并重建 Huffman 树，随后解码数据
	void decompress(const std::filesystem::path& src_path, const std::filesystem::path& dst_path, bool show_rate, bool show_tree) {
		if (!src_path.string().ends_with(".huff")) {
			throw std::runtime_error("请选择.huff文件：" + src_path.string());
		}
		std::ifstream ifs(src_path, std::ios::binary);
		if (!ifs.is_open()) {
			throw std::runtime_error("文件打开失败：" + src_path.string());
		}

		// 读取树的位数与字节数并读取对应字节
		std::size_t tree_bit_count = 0, tree_byte_count = 0;
		ifs.read(reinterpret_cast<char*>(&tree_bit_count), sizeof(tree_bit_count));
		ifs.read(reinterpret_cast<char*>(&tree_byte_count), sizeof(tree_byte_count));
		std::vector<byte> tree_data;
		try {
			tree_data = std::vector<byte>(tree_byte_count);
		}
		catch (std::bad_alloc b) {
			throw std::runtime_error("错误的.huff压缩文件");
		}
		ifs.read(reinterpret_cast<char*>(tree_data.data()), tree_byte_count);
		// 用读取到的数据重构 byte_array 并反序列化树
		byte_array tree_structure(tree_data, tree_bit_count);
		huffman_tree tree(tree_structure);
		if (show_tree) {
			tree.print_as_tree(1);
		}

		// 读取编码数据的位数与字节数并读取对应字节
		std::size_t bit_count = 0, byte_count = 0;
		ifs.read(reinterpret_cast<char*>(&bit_count), sizeof(bit_count));
		ifs.read(reinterpret_cast<char*>(&byte_count), sizeof(byte_count));
		std::vector<byte> compressed_data;
		try {
			compressed_data = std::vector<byte>(byte_count);
		}
		catch (std::bad_alloc b) {
			throw std::runtime_error("错误的.huff压缩文件");
		}
		ifs.read(reinterpret_cast<char*>(compressed_data.data()), byte_count);
		byte_array compressed(compressed_data, bit_count);

		// 解码
		auto decompressed = tree.decode(compressed);
		std::ofstream ofs(dst_path, std::ios::binary);
		if (!ofs.is_open()) {
			throw std::runtime_error("无法创建解压文件: " + dst_path.string());
		}
		ofs.write(reinterpret_cast<const char*>(decompressed.data()), decompressed.size());
		ofs.close();

		// 输出压缩率信息
		if (show_rate) {
			auto src_size = std::filesystem::file_size(src_path);
			auto dst_size = std::filesystem::file_size(dst_path);
			double decompression_ratio = (1 - (double)dst_size / src_size) * 100;
			std::ostringstream oss;
			oss << "原始文件大小：" << src_size / 1024.0 << " KB\n";
			oss << "解压缩文件大小：" << dst_size / 1024.0 << " KB\n";
			oss << "实际解压缩率：" << std::fixed << std::setprecision(2) << decompression_ratio << "%\n";
			std::cout << oss.str();
		}
	}

	// 基于位数组内容计算哈希（用于在 unordered_map 中作为键）
	size_t byte_array_hash::operator()(const byte_array& binary) const {
		size_t seed = binary.size();
		for (byte b : binary.data()) {
			seed ^= std::hash<byte>{}(b)+0x9e3779b9 + (seed << 6) + (seed >> 2);
		}
		return seed;
	}

	// 使用频率表构建 Huffman 树（最小堆合并节点）
	void huffman_tree::build_tree(const std::unordered_map<byte, unsigned>& frequency_table) {
		std::priority_queue<
			std::shared_ptr<huffman_node>,
			std::vector<std::shared_ptr<huffman_node>>,
			huffman_node_comparator
		> min_heap;
		for (const auto& pair : frequency_table) {
			min_heap.push(std::make_shared<huffman_node>(pair.first, pair.second));
		}
		if (min_heap.empty()) {
			m_root = nullptr;
			return;
		}
		// 处理只有一个符号的特殊情况：创建父节点以确保树有根且编码有效
		if (min_heap.size() == 1) {
			std::shared_ptr<huffman_node> left = min_heap.top();
			min_heap.pop();
			m_root = std::make_shared<huffman_node>(left->frequency, left, nullptr);
			return;
		}
		// 常规合并直到只剩根节点
		while (min_heap.size() > 1) {
			std::shared_ptr<huffman_node> left = min_heap.top();
			min_heap.pop();
			std::shared_ptr<huffman_node> right = min_heap.top();
			min_heap.pop();
			size_t sum_freq = left->frequency + right->frequency;
			auto parent = std::make_shared<huffman_node>(sum_freq, left, right);
			min_heap.push(parent);
		}
		m_root = min_heap.top();
	}

	// 根据原始字节向量统计频率表
	std::unordered_map<byte, unsigned> huffman_tree::build_frequency_table(const std::vector<byte>& vec_data) {
		std::unordered_map<byte, unsigned> frequency_table;
		for (byte data : vec_data) {
			frequency_table[data]++;
		}
		return frequency_table;
	}

	// 从已知频率表构建树并生成编码表
	void huffman_tree::from_frequency_table(const std::unordered_map<byte, unsigned>& frequency_table) {
		build_tree(frequency_table);
		generate_codes(m_root, byte_array());
	}

	// 从向量直接构建（频率表 -> 树 -> 编码）
	void huffman_tree::from_vector(const std::vector<byte>& vec_data) {
		auto frequency_table = build_frequency_table(vec_data);
		build_tree(frequency_table);
		generate_codes(m_root, byte_array());
	}

	// 从序列化的二进制数据重建树并生成编码表
	void huffman_tree::from_binary_data(const byte_array& serialized_tree) {
		size_t bit_index = 0;
		byte_array copy_data = serialized_tree;
		m_root = deserialize_tree(copy_data, bit_index);
		generate_codes(m_root, byte_array());
	}

	// 递归生成每个符号的编码（当前路径存为 bit 序列）
	void huffman_tree::generate_codes(std::shared_ptr<huffman_node> node, const byte_array& current_code) {
		if (node == nullptr) return;
		if (node->is_leaf()) {
			byte_array code = current_code;
			// 若只有一个符号，确保至少有一个比特（避免空编码）
			if (code.empty()) {
				code.push_back(0);
			}
			m_codes[node->data] = code;
			m_reverse_codes[code] = node->data;
		}
		else {
			// 左子树用 0，右子树用 1（约定）
			byte_array left_code = current_code;
			left_code.push_back(0);
			generate_codes(node->left, left_code);
			byte_array right_code = current_code;
			right_code.push_back(1);
			generate_codes(node->right, right_code);
		}
	}

	// 按位遍历树以解码一个符号；从给定节点出发直到遇到叶子
	byte huffman_tree::decode_single(std::shared_ptr<huffman_node> node, const byte_array& encoded, size_t& bit_index) const {
		while (!node->is_leaf()) {
			if (bit_index >= encoded.size()) {
				throw std::invalid_argument("无效编码");
			}
			bool bit = encoded.bit(bit_index++);
			if (bit) {
				node = node->right;
			}
			else {
				node = node->left;
			}
		}
		return node->data;
	}

	// 序列化树结构（前序）：叶子节点输出 1 + 8bit 数据，内部节点输出 0
	void huffman_tree::serialize_tree(std::shared_ptr<huffman_node> node, byte_array& buffer) const {
		if (node == nullptr) return;
		if (node->is_leaf()) {
			buffer.push_back(1);
			serialize_data(node->data, buffer);
		}
		else {
			buffer.push_back(0);
			serialize_tree(node->left, buffer);
			serialize_tree(node->right, buffer);
		}
	}

	// 将一个字节按高位到低位写入到 bit 缓冲（用于序列化叶子数据）
	void huffman_tree::serialize_data(byte data, byte_array& buffer) const {
		for (int i = 7; i >= 0; --i) {
			buffer.push_back((data >> i) & 1);
		}
	}

	// 从序列化的位流反序列化树（与 serialize_tree 对应）
	std::shared_ptr<huffman_node> huffman_tree::deserialize_tree(byte_array& buffer, size_t& bit_index) const {
		if (bit_index >= buffer.size()) return nullptr;
		// 1 表示叶子，随后读取 8bit 数据；0 表示内部节点并递归读取子树
		if (buffer.bit(bit_index++)) {
			byte data = deserialize_data(buffer, bit_index);
			return std::make_shared<huffman_node>(data, 0);
		}
		else {
			auto left = deserialize_tree(buffer, bit_index);
			auto right = deserialize_tree(buffer, bit_index);
			return std::make_shared<huffman_node>(0, left, right);
		}
	}

	// 从位流中读取 8 位还原为一个字节（用于反序列化叶子数据）
	byte huffman_tree::deserialize_data(byte_array& buffer, size_t& bit_index) const {
		byte data = 0;
		for (int i = 7; i >= 0; --i) {
			if (bit_index >= buffer.size()) {
				throw std::runtime_error("预期长度外的树数据");
			}
			if (buffer.bit(bit_index++)) {
				data |= (1 << i);
			}
		}
		return data;
	}

	// 前序遍历打印节点信息（用于生成字符串表示）
	void huffman_tree::prefind(std::shared_ptr<huffman_node> node, std::string& buffer, bool show_code) const {
		if (node == nullptr) return;
		if (node->is_leaf()) {
			if (show_code) {
				buffer += "[" + chr::to_string(node->data) + "]:" + encode(node->data).to_string() + " ";
			}
			else {
				buffer += "[" + chr::to_string(node->data) + "] ";
			}
		}
		else {
			buffer += "{" + std::to_string(node->frequency) + "} ";
		}
		prefind(node->left, buffer, show_code);
		prefind(node->right, buffer, show_code);
	}

	// 中序遍历打印
	void huffman_tree::infind(std::shared_ptr<huffman_node> node, std::string& buffer, bool show_code) const {
		if (node == nullptr) return;
		prefind(node->left, buffer, show_code);
		if (node->is_leaf()) {
			if (show_code) {
				buffer += "[" + chr::to_string(node->data) + "]:" + encode(node->data).to_string() + " ";
			}
			else {
				buffer += "[" + chr::to_string(node->data) + "] ";
			}
		}
		else {
			buffer += "{" + std::to_string(node->frequency) + "} ";
		}
		prefind(node->right, buffer, show_code);
	}

	// 后序遍历打印
	void huffman_tree::postfind(std::shared_ptr<huffman_node> node, std::string& buffer, bool show_code) const {
		if (node == nullptr) return;
		prefind(node->left, buffer, show_code);
		prefind(node->right, buffer, show_code);
		if (node->is_leaf()) {
			if (show_code) {
				buffer += "[" + chr::to_string(node->data) + "]:" + encode(node->data).to_string() + " ";
			}
			else {
				buffer += "[" + chr::to_string(node->data) + "] ";
			}
		}
		else {
			buffer += "{" + std::to_string(node->frequency) + "} ";
		}
	}

	// 以树状结构友好地在控制台打印 Huffman 树（递归辅助）
	void huffman_tree::print_as_tree_helper(std::shared_ptr<huffman_node> node, const std::string& prefix, bool is_left, bool show_code) const {
		if (node == nullptr) return;
		std::cout << prefix;
		std::cout << (is_left ? "├──" : "└──");
		if (node->is_leaf()) {
			if (show_code) {
				std::cout << "[" + chr::to_string(node->data) + "]:" + encode(node->data).to_string() + "\n";
			}
			else {
				std::cout << "[" + chr::to_string(node->data) + "]\n";
			}
		}
		else {
			std::cout << "{" + std::to_string(node->frequency) + "}\n";
		}
		std::string new_prefix = prefix + (is_left ? "│   " : "    ");
		print_as_tree_helper(node->left, new_prefix, 1, show_code);
		print_as_tree_helper(node->right, new_prefix, 0, show_code);
	}

	// 根据单字节返回对应编码（若未找到则抛出异常）
	const byte_array& huffman_tree::encode(byte data) const {
		auto it = m_codes.find(data);
		if (it != m_codes.end()) {
			return it->second;
		}
		throw std::invalid_argument("未找到相应编码");
	}

	// 将整个字节向量编码为位数组（拼接每个字节的编码）
	byte_array huffman_tree::encode(const std::vector<byte>& vec_data) const {
		byte_array result;
		for (byte data : vec_data) {
			result += encode(data);
		}
		return result;
	}

	// 编码并返回一些统计信息（用于输出压缩效果）
	std::pair<byte_array, std::string> huffman_tree::encode_with_info(const std::vector<byte>& vec_data) const {
		byte_array encoded = encode(vec_data);
		size_t original_size = vec_data.size() * 8;
		size_t encoded_size = encoded.size();
		double compression_ratio = (1 - (double)encoded_size / original_size) * 100;
		std::ostringstream oss;
		oss << "数据数量：" << vec_data.size() << "\n";
		oss << "原始大小：" << original_size << " 位\n";
		oss << "编码大小：" << encoded_size << " 位\n";
		oss << "压缩率：" << std::fixed << std::setprecision(2) << compression_ratio << "%\n";
		return { encoded,oss.str() };
	}

	// 通用解码：逐位使用树结构解出原始字节
	std::vector<byte> huffman_tree::decode(const byte_array& encoded) const{
		std::vector<byte> result;
		if (m_root == nullptr || encoded.empty()) return result;
		auto it = std::back_inserter(result);
		// 若仅含单个符号的树，直接重复该符号以对应编码位数
		if (m_root->is_leaf()) {
			for (std::size_t i = 0; i < encoded.size(); ++i) {
				*it++ = m_root->data;
			}
			return result;
		}
		std::size_t bit_index = 0;
		while (bit_index < encoded.size()) {
			*it++ = decode_single(m_root, encoded, bit_index);
		}
		return result;
	}

	// 基于编码表的快速解码：通过逐位累积 code 并在反向映射表中查找
	std::vector<byte> huffman_tree::fast_decode(const byte_array& encoded) const {
		std::vector<byte> result;
		auto it = std::back_inserter(result);
		byte_array current_code;
		for (std::size_t i = 0; i < encoded.size(); ++i) {
			current_code.push_back(encoded.bit(i));
			auto at = m_reverse_codes.find(current_code);
			if (at != m_reverse_codes.end()) {
				*it++ = at->second;
				current_code.clear();
			}
		}
		if (!current_code.empty()) {
			throw std::invalid_argument("不完整的编码");
		}
		return result;
	}

	// 将树序列化为位数组并返回
	byte_array huffman_tree::to_byte_array() const {
		byte_array buffer;
		serialize_tree(m_root, buffer);
		return buffer;
	}

	// 根据遍历模式返回树的字符串表示（用于调试/显示）
	std::string huffman_tree::to_string(traversal_mode mode, bool show_code) const {
		std::string buffer;
		switch (mode) {
		case traversal_mode::preorder:
			prefind(m_root, buffer, show_code);
			break;
		case traversal_mode::inorder:
			infind(m_root, buffer, show_code);
			break;
		case traversal_mode::postorder:
			postfind(m_root, buffer, show_code);
			break;
		default:
			throw std::invalid_argument("未知遍历模式");
		}
		return buffer;
	}

	// 打印为树状结构到控制台（入口）
	void huffman_tree::print_as_tree(bool show_code) const {
		print_as_tree_helper(m_root, "", 1, show_code);
	}

	// 输出编码表（每个符号及其对应编码）
	std::string huffman_tree::code_table() const {
		std::ostringstream oss;
		for (const auto& pair : m_codes) {
			oss << "[" << chr::to_string(pair.first) << "]:" << pair.second.to_string() << "\n";
		}
		return oss.str();
	}

}