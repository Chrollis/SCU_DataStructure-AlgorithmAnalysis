#include "chrtoken.h"

namespace chr {
	token_t string_to_operator_token_type(const std::string& operator_str)
	{
		std::vector<operator_token*> exist_ops = {
			new left_parentheses_token,
			new right_parentheses_token,
			new plus_token,
			new minus_token,
			new multiply_token,
			new divide_token,
			new posite_token,
			new negate_token,
			new exponent_token,
			new sine_token,
			new cosine_token,
			new tangent_token,
			new cotangent_token,
			new secant_token,
			new cosecant_token,
			new arcsine_token,
			new arccosine_token,
			new arctangent_token,
			new arccotangent_token,
			new arcsecant_token,
			new arccosecant_token,
			new common_logarithm_token,
			new natural_logarithm_token,
			new square_root_token,
			new cubic_root_token,
			new factorial_token,
			new modulo_token,
			new degree_token,
			new radian_token
		};
		token_t type = token_invalid;
		for (const auto& op : exist_ops) {
			if (op->str() == operator_str) {
				type = op->type();
				break;
			}
		}
		if (type == token_invalid) {
			throw std::runtime_error("未知的运算符令牌字符串");
		}
		for (auto op : exist_ops) {
			delete op;
		}
		return type;
	}

	operator_token* token_type_to_operator_token(token_t operator_type)
	{
		switch (operator_type) {
		case token_left_parentheses:
			return new left_parentheses_token;
		case token_right_parentheses:
			return new right_parentheses_token;
		case token_plus:
			return new plus_token;
		case token_minus:
			return new minus_token;
		case token_multiply:
			return new multiply_token;
		case token_divide:
			return new divide_token;
		case token_posite:
			return new posite_token;
		case token_negate:
			return new negate_token;
		case token_exponent:
			return new exponent_token;
		case token_sine:
			return new sine_token;
		case token_cosine:
			return new cosine_token;
		case token_tangent:
			return new tangent_token;
		case token_cotangent:
			return new cotangent_token;
		case token_secant:
			return new secant_token;
		case token_cosecant:
			return new cosecant_token;
		case token_arcsine:
			return new arcsine_token;
		case token_arccosine:
			return new arccosine_token;
		case token_arctangent:
			return new arctangent_token;
		case token_arccotangent:
			return new arccotangent_token;
		case token_arcsecant:
			return new arcsecant_token;
		case token_arccosecant:
			return new arccosecant_token;
		case token_common_logarithm:
			return new common_logarithm_token;
		case token_natural_logarithm:
			return new natural_logarithm_token;
		case token_square_root:
			return new square_root_token;
		case token_cubic_root:
			return new cubic_root_token;
		case token_factorial:
			return new factorial_token;
		case token_modulo:
			return new modulo_token;
		case token_degree:
			return new degree_token;
		case token_radian:
			return new radian_token;
		default:
			throw std::runtime_error("未知的运算符令牌类型");
		}
	}
};