#ifndef CHREXPRESSION_H
#define CHREXPRESSION_H

#include "chrtoken.h"
#include "chrvalidator.h"
#include <sstream>

namespace chr {
	class basic_expression {
	protected:
		std::vector<basic_token*> _content;
		static void calculate(std::stack<number_token>& operand_stack, const operator_token& op);
	public:
		basic_expression() = default;
		std::vector<basic_token*> content()const { return _content; }
		virtual ~basic_expression();
		virtual double evaluate()const = 0;
	};

	std::ostream& operator<<(std::ostream& os, const basic_expression& expr);

	class infix_expression :public basic_expression {
	public:
		infix_expression(const std::string& infix_expr_str);
		~infix_expression() = default;
		double evaluate()const;
	};

	class postfix_expression :public basic_expression {
	public:
		postfix_expression(const std::string& infix_expr_str);
		~postfix_expression() = default;
		double evaluate()const;
	};
};

#endif
