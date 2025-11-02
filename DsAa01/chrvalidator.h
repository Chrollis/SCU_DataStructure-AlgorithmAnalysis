#ifndef CHRVALIDATOR_H
#define CHRVALIDATOR_H

#include <iostream>
#include <string>
#include <regex>
#include <vector>
#include <stack>
#include <algorithm>

namespace chr {
	class expression_tokenizer {
	private:
		std::vector<std::string> _tokens;
		std::vector<std::pair<std::string, std::string>> _errors;
		static const std::vector<std::string> functions;
		static const std::vector<std::string> constants;
	public:
		static bool is_operator(const std::string& token);
		static bool is_function(const std::string& token);
		static bool is_constant(const std::string& token);
		static bool is_number(const std::string& token);
		static std::string get_token_type(const std::string& token);
	private:
		void proceess_unary_operators();
	public:
		bool tokenize(const std::string& expr);
		void print_tokens(std::ostream& os)const;
		void print_errors(std::ostream& os)const;
		void add_error(const std::pair<std::string, std::string>& error);
		const std::vector<std::string>& tokens()const { return _tokens; }
		const std::vector<std::pair<std::string, std::string>>& errors()const { return _errors; }
	};

	class expression_validator {
	private:
		expression_tokenizer _tokenizer;
	private:
		void check_parentheses(const std::vector<std::string>& tokens);
		void check_operator_sequence(const std::vector<std::string>& tokens);
		void check_number_format(const std::vector<std::string>& tokens);
		void check_function_usage(const std::vector<std::string>& tokens);
	public:
		bool validate_expression(const std::string& expr);
		void print_detailed_analysis(std::ostream& os)const;
		const expression_tokenizer& tokenizer() { return _tokenizer; }
	};
}

#endif // CHRVALIDATOR_H
