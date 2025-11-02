#include "chrvalidator.h"

namespace chr {
	const std::vector<std::string> expression_tokenizer::functions = {
		   "sin", "cos", "tan", "cot", "sec", "csc",
		   "arcsin", "arccos", "arctan", "arccot", "arcsec", "arccsc",
		   "ln", "lg", "deg", "rad", "sqrt", "cbrt"
	};
	const std::vector<std::string> expression_tokenizer::constants = { 
		"PI", "E", "PHI" 
	};

	bool expression_tokenizer::is_operator(const std::string& token) {
		return std::regex_match(token, std::regex(R"([+\-*/^()!%])"));
	}

	bool expression_tokenizer::is_function(const std::string& token) {
		return std::find(functions.begin(), functions.end(), token) != functions.end();
	}

	bool expression_tokenizer::is_constant(const std::string& token) {
		return std::find(constants.begin(), constants.end(), token) != constants.end();
	}

	bool expression_tokenizer::is_number(const std::string& token)
	{
		std::regex pattern(
			R"((0b[01]+(\.[01]*)?)|)"
			R"((0o[0-7]+(\.[0-7]*)?)|)"
			R"((0x[0-9A-Fa-f]+(\.[0-9A-Fa-f]*)?)|)"
			R"((\d+\.?\d*|\.\d+)([eE][-+]?\d+)?|)"
			R"(PI|E|PHI|)"
		);
		return std::regex_match(token, pattern);
	}

	void expression_tokenizer::proceess_unary_operators() {
		std::vector<std::string> processed_tokens;
		for (size_t i = 0; i < _tokens.size(); ++i) {
			const std::string& token = _tokens[i];
			if (token == "+" || token == "-") {
				if (i == 0 || (i > 0 && (is_operator(_tokens[i - 1]) && _tokens[i - 1] != ")" && _tokens[i - 1] != "!") || is_function(_tokens[i - 1]))) {
					processed_tokens.push_back(token == "+" ? "pos" : "neg");
				}
				else {
					processed_tokens.push_back(token);
				}
			}
			else {
				processed_tokens.push_back(token);
			}
		}
		_tokens = processed_tokens;
	}

	std::string expression_tokenizer::get_token_type(const std::string& token){
		if (std::regex_match(token, std::regex(R"(0b[01]+(\.[01]*)?)"))) {
			return "BINARY";
		}
		else if (std::regex_match(token, std::regex(R"(0o[0-7]+(\.[0-7]*)?)"))) {
			return "OCTAL";
		}
		else if (std::regex_match(token, std::regex(R"(0x[0-9A-Fa-f]+(\.[0-9A-Fa-f]*)?)"))) {
			return "HEXADECIMAL";
		}
		else if (std::regex_match(token, std::regex(R"((\d+\.?\d*|\.\d+)([eE][-+]?\d+)?)"))) {
			return "DECIMAL";
		}
		else if (is_operator(token)) {
			return "OPERATOR";
		}
		else if (is_constant(token)) {
			return "CONSTANT";
		}
		else if (is_function(token)) {
			return "FUNCTION";
		}
		else if (token == "pos" || token == "neg") {
			return "UNARY_OPERATOR";
		}
		return "UNKNOWN";
	}

	bool expression_tokenizer::tokenize(const std::string& expr) {
		_tokens.clear();
		_errors.clear();
		std::string processed_expr = expr;
		std::regex pattern(
			R"((0b[01]+(\.[01]*)?)|)"
			R"((0o[0-7]+(\.[0-7]*)?)|)"
			R"((0x[0-9A-Fa-f]+(\.[0-9A-Fa-f]*)?)|)"
			R"((\d+\.?\d*|\.\d+)([eE][-+]?\d+)?|)"
			R"([+\-*/^()!%]|)"
			R"(PI|E|PHI|)"
			R"(sin|cos|tan|cot|sec|csc|)"
			R"(arcsin|arccos|arctan|arccot|arcsec|arccsc|)"
			R"(ln|lg|deg|rad|sqrt|cbrt|)"
		);
		size_t pos = 0;
		auto words_begin = std::sregex_iterator(processed_expr.begin(), processed_expr.end(), pattern);
		auto words_end = std::sregex_iterator();
		for (auto& it = words_begin; it != words_end; it++) {
			std::smatch match = *it;
			std::string token = match.str();
			size_t token_pos = match.position();
			if (std::all_of(token.begin(), token.end(), isspace)) {
				continue;
			}
			if (token_pos > pos) {
				std::string unknown = processed_expr.substr(pos, token_pos - pos);
				if (!std::all_of(unknown.begin(), unknown.end(), isspace)) {
					_errors.push_back({ unknown,"无法识别的字符或符号" });
				}
			}
			_tokens.push_back(token);
			pos = token_pos + token.length();
		}
		if (pos < processed_expr.length()) {
			std::string remaining = processed_expr.substr(pos);
			if (!std::all_of(remaining.begin(), remaining.end(), isspace)) {
				_errors.push_back({ remaining, "表达式末尾有无法识别的字符" });
			}
		}
		proceess_unary_operators();
		return _errors.empty();
	}

	void expression_tokenizer::print_tokens(std::ostream& os) const {
		for (const auto& token : _tokens) {
			std::string type = get_token_type(token);
			os << "[" << type << "] " << token << "\n";
		}
	}

	void expression_tokenizer::print_errors(std::ostream& os) const {
		for (const auto& error : _errors) {
			os << "位置【" + error.first + "】：" + error.second + "\n";
		}
	}

	void expression_tokenizer::add_error(const std::pair<std::string, std::string>& error) {
		_errors.push_back(error);
	}

	void expression_validator::check_parentheses(const std::vector<std::string>& tokens) {
		std::stack<std::pair<std::string, size_t>>paren_stack;
		for (size_t i = 0; i < tokens.size(); i++) {
			const auto& token = tokens[i];
			if (token == "(") {
				paren_stack.push({ token,i });
			}
			else if (token == ")") {
				if (paren_stack.empty()) {
					_tokenizer.add_error({ std::to_string(i),"存在多余的右括弧" });
				}
				else {
					paren_stack.pop();
				}
			}
		}
		while (!paren_stack.empty()) {
			_tokenizer.add_error({ std::to_string(paren_stack.top().second),"存在多余的左括弧" });
			paren_stack.pop();
		}
	}

	void expression_validator::check_operator_sequence(const std::vector<std::string>& tokens) {
		const std::vector<std::string> binary_operators = { "+", "-", "*", "/", "^", "%" };
		const std::vector<std::string> unary_operators = { "pos","neg" };
		for (size_t i = 0; i < tokens.size(); ++i) {
			const std::string& token = tokens[i];
			if (std::find(binary_operators.begin(), binary_operators.end(), token) != binary_operators.end()) {
				if (i == 0) {
					_tokenizer.add_error({ std::to_string(i),"表达式以二元运算符开头" });
				}
				else if (i == tokens.size() - 1) {
					_tokenizer.add_error({ std::to_string(i),"表达式以运算符结尾" });
				}
				else {
					if (std::find(binary_operators.begin(), binary_operators.end(), tokens[i - 1]) != binary_operators.end()) {
						_tokenizer.add_error({ std::to_string(i),"表达式含有连续二元运算符" });
					}
				}
			}
			if (std::find(unary_operators.begin(), unary_operators.end(), token) != unary_operators.end()) {
				if (i == tokens.size() - 1) {
					_tokenizer.add_error({ std::to_string(i),"表达式以运算符结尾" });
				}
				else {
					if (i != 0 && std::find(unary_operators.begin(), unary_operators.end(), tokens[i - 1]) != unary_operators.end()) {
						_tokenizer.add_error({ std::to_string(i),"表达式含有连续一元运算符" });
					}
				}
			}
			if (token == "!") {
				if (i == 0) {
					_tokenizer.add_error({ std::to_string(i),"表达式以阶乘运算符开头" });
				}
				else {
					const std::string& prev = tokens[i - 1];
					if (!(std::regex_match(prev, std::regex(R"((\d+\.?\d*|\.\d+)([eE][-+]?\d+)?)")) ||
						std::regex_match(prev, std::regex(R"(0[bxo][0-9A-Fa-f.]+)")) ||
						prev == ")" || prev == "PI" || prev == "E" || prev == "PHI")) {
						_tokenizer.add_error({ std::to_string(i),"阶乘运算符前面必须是数字、常量或表达式" });
					}
				}
			}
		}
	}

	void expression_validator::check_number_format(const std::vector<std::string>& tokens) {
		for (size_t i = 0; i < tokens.size(); i++) {
			const auto& token = tokens[i];
			if (expression_tokenizer::is_number(token) && !expression_tokenizer::is_constant(token)) {
				if (i > 0 && expression_tokenizer::is_number(tokens[i - 1])) {
					_tokenizer.add_error({ tokens[i - 1] + token,"表达式含有连续数字" });
				}
				else {
					if ((token.find('e') != std::string::npos || token.find('E') != std::string::npos) &&
						!token.starts_with("0x") && !token.starts_with("0o") && !token.starts_with("0b")) {
						if (!std::regex_match(token, std::regex(R"([+-]?(\d+\.?\d*|\.\d+)[eE][-+]?\d+)"))) {
							_tokenizer.add_error({ token,"科学计数法格式错误" });
						}
					}
					if (token.starts_with("0b") &&
						!std::regex_match(token, std::regex(R"(0b[01]+(\.[01]*)?)"))) {
						_tokenizer.add_error({ token,"二进制格式错误" });
					}
					else if (token.starts_with("0o") &&
						!std::regex_match(token, std::regex(R"(0o[0-7]+(\.[0-7]*)?)"))) {
						_tokenizer.add_error({ token,"八进制格式错误" });
					}
					else if (token.starts_with("0x") &&
						!std::regex_match(token, std::regex(R"(0x[0-9A-Fa-f]+(\.[0-9A-Fa-f]*)?)"))) {
						_tokenizer.add_error({ token,"十六进制格式错误" });
					}
				}
			}
		}
	}

	void expression_validator::check_function_usage(const std::vector<std::string>& tokens) {
		const std::vector<std::string> functions = {
			"sin", "cos", "tan", "cot", "sec", "csc",
			"arcsin", "arccos", "arctan", "arccot", "arcsec", "arccsc",
			"ln", "lg", "deg", "rad", "sqrt", "cbrt"
		};
		for (size_t i = 0; i < tokens.size(); ++i) {
			const std::string& token = tokens[i];
			if (std::find(functions.begin(), functions.end(), token) != functions.end()) {
				if (i + 1 >= tokens.size() || tokens[i + 1] != "(") {
					_tokenizer.add_error({ token,"函数名未紧跟左括号" });
				}
			}
		}
	}

	bool expression_validator::validate_expression(const std::string& expr) {
		if (!_tokenizer.tokenize(expr)) {
			return 0;
		}
		const auto& tokens = _tokenizer.tokens();
		check_parentheses(tokens);
		check_operator_sequence(tokens);
		check_number_format(tokens);
		check_function_usage(tokens);
		const auto& errors = _tokenizer.errors();
		return errors.empty();
	}

	void expression_validator::print_detailed_analysis(std::ostream& os) const {
		_tokenizer.print_tokens(os);
		_tokenizer.print_errors(os);
	}

}
