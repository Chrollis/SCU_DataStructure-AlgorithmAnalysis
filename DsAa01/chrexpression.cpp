#include "chrexpression.h"

namespace chr {

	void basic_expression::calculate(std::stack<number_token>& operand_stack, const operator_token& op)
	{
		int operands = op.operand_num();
		if (operands == 0) {
			throw std::runtime_error("计算时出现零操作数运算符");
		}
		else if (operands == 1) {
			double left = operand_stack.top().value();
			operand_stack.top().rvalue() = op.apply(left, 0);
		}
		else if (operands == 2) {
			double right = operand_stack.top().value();
			operand_stack.pop();
			double left = operand_stack.top().value();
			operand_stack.top().rvalue() = op.apply(left, right);
		}
		else {
			throw std::runtime_error("计算时出现操作数多于两个的运算符");
		}
	}

	basic_expression::~basic_expression()
	{
		for (auto token : _content) {
			delete token;
		}
	}

	std::ostream& operator<<(std::ostream& os, const basic_expression& expr) 
	{
		std::vector<basic_token*> token_list = expr.content();
		for (const basic_token* const token : token_list) {
			if (token->type() == token_number) {
				os << dynamic_cast<const number_token*>(token)->value() << ' ';
			}
			else {
				os << dynamic_cast<const operator_token*>(token)->str() << ' ';
			}
		}
		return os;
	}

	infix_expression::infix_expression(const std::string& infix_expr_str)
	{
		expression_validator validator;
		if (!validator.validate_expression(infix_expr_str)) {
			std::ostringstream oss;
			oss << "表达式非法：\n";
			validator.print_detailed_analysis(oss);
			std::string error = oss.str();
			error.pop_back();
			throw std::runtime_error(error);
		}
		std::vector<std::string> tokens = validator.tokenizer().tokens();
		for (const auto& token : tokens) {
			if (expression_tokenizer::is_number(token)) {
				std::string type = expression_tokenizer::get_token_type(token);
				if (type == "DECIMAL") {
					_content.push_back(new number_token(std::stod(token)));
				}
				else if (type == "CONSTANT") {
					if (token == "E") {
						_content.push_back(new number_token(natural_constant));
					}
					else if (token == "PI") {
						_content.push_back(new number_token(pi));
					}
					else if (token == "PHI") {
						_content.push_back(new number_token(phi));
					}
					else {
						throw std::runtime_error("无效常数");
					}
				}
				else {
					double out = 0;
					int radix = 10;
					std::string integer;
					std::string fraction;
					if (type == "BINARY") {
						radix = 2;
					}
					else if (type == "OCTAL") {
						radix = 8;
					}
					else if (type == "HEXADECIMAL") {
						radix = 16;
					}
					else {
						throw std::runtime_error("无效进制");
					}
					size_t dot_pos = token.find('.');
					if (dot_pos == std::string::npos) {
						integer = token.substr(2);
					}
					else {
						integer = token.substr(2, dot_pos - 2);
						fraction = token.substr(dot_pos + 1);
					}
					for (size_t i = 0; i < integer.length(); i++) {
						char t = integer[integer.length() - i - 1];
						out += pow(radix, i) * (t <= '9' ? t - '0' : t <= 'Z' ? t - 'A' + 10 : t - 'a' + 10);
					}
					for (size_t i = 0; i < fraction.length(); i++) {
						char t = fraction[i];
						out += pow(radix, -(int(i) + 1)) * (t <= '9' ? t - '0' : t <= 'Z' ? t - 'A' + 10 : t - 'a' + 10);
					}
					_content.push_back(new number_token(out));
				}
				continue;
			}
			token_t type = string_to_operator_token_type(token);
			_content.push_back(token_type_to_operator_token(type));
		}
	}

	double infix_expression::evaluate()const {
		std::stack<number_token> operand_stack;
		std::stack<operator_token*> operator_stack;
		for (auto token : _content) {
			if (token->type() == token_number) {
				operand_stack.push(number_token(dynamic_cast<number_token*>(token)->value()));
			}
			else {
				operator_token* op = dynamic_cast<operator_token*>(token);
				if (op->type() == token_left_parentheses) {
					operator_stack.push(token_type_to_operator_token(op->type()));
				}
				else if (op->type() == token_right_parentheses) {
					while (!operator_stack.empty()) {
						if (operator_stack.top()->type() == token_left_parentheses) {
						    delete operator_stack.top();
							operator_stack.pop();
							break;
						}
						else {
							calculate(operand_stack, *operator_stack.top());
							delete operator_stack.top();
							operator_stack.pop();
						}
					}
				}
				else {
					while (!operator_stack.empty() && operator_stack.top()->priority() >= op->priority()) {
						calculate(operand_stack, *operator_stack.top());
						delete operator_stack.top();
						operator_stack.pop();
					}
					operator_stack.push(token_type_to_operator_token(op->type()));
				}
			}
		}
		while (!operator_stack.empty()) {
			calculate(operand_stack, *operator_stack.top());
			delete operator_stack.top();
			operator_stack.pop();
		}
		if (operand_stack.size() != 1) {
			throw std::runtime_error("运算结束时出错，操作数栈不只有一个元素");
		}
		else {
			return operand_stack.top().value();
		}
	}

	postfix_expression::postfix_expression(const std::string& infix_expr_str)
	{
		infix_expression* infix_expr = new infix_expression(infix_expr_str);
		std::vector<basic_token*> infix_tokens = infix_expr->content();
		std::stack<operator_token*> op_stack;
		for (auto token : infix_tokens) {
			int type = token->type();
			if (type == token_number) {
				_content.push_back(new number_token(dynamic_cast<number_token*>(token)->value()));
			}
			else {
				operator_token* op = dynamic_cast<operator_token*>(token);
				if (op->type() == token_left_parentheses) {
					op_stack.push(op);
				}
				else if (type == token_right_parentheses) {
					while (!op_stack.empty()) {
						if (op_stack.top()->type() == token_left_parentheses) {
							op_stack.pop();
							break;
						}
						else {
							_content.push_back(token_type_to_operator_token(op_stack.top()->type()));
							op_stack.pop();
						}
					}
				}
				else {
					while (!op_stack.empty() && op_stack.top()->priority() >= op->priority()) {
						_content.push_back(token_type_to_operator_token(op_stack.top()->type()));
						op_stack.pop();
					}
					op_stack.push(dynamic_cast<operator_token*>(token));
				}
			}
		}
		while (!op_stack.empty()) {
			_content.push_back(token_type_to_operator_token(op_stack.top()->type()));
			op_stack.pop();
		}
		delete infix_expr;
	}

	double postfix_expression::evaluate()const {
		std::stack<number_token> operand_stack;
		for (auto token : _content) {
			if (token->type() == token_number) {
				operand_stack.push(number_token(dynamic_cast<number_token*>(token)->value()));
			}
			else {
				calculate(operand_stack, *dynamic_cast<operator_token*>(token));
			}
		}
		if (operand_stack.size() != 1) {
			throw std::runtime_error("运算结束时出错，操作数栈不只有一个元素");
		}
		else {
			return operand_stack.top().value();
		}
	}
}