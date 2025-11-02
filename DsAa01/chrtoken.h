#ifndef CHRTOKEN_H
#define CHRTOKEN_H

#include <string>
#include <cmath>
#include <stdexcept>
#include <vector>

namespace chr {
	constexpr double natural_constant = 2.718281828459;
	constexpr double pi = 3.1415926535898;
	constexpr double phi = 0.61803398875;

	typedef unsigned short token_t;
	typedef unsigned char byte;

	class basic_token {
	protected:
		token_t _type;
	public:
		basic_token(token_t type) :_type(type) {}
		virtual ~basic_token() {};
		token_t type()const { return _type; }
	};

	enum token_type {
		token_invalid,
		token_number,
		token_left_parentheses,
		token_right_parentheses,
		token_plus,
		token_minus,
		token_multiply,
		token_divide,
		token_posite,
		token_negate,
		token_exponent,
		token_sine,
		token_cosine,
		token_tangent,
		token_cotangent,
		token_secant,
		token_cosecant,
		token_arcsine,
		token_arccosine,
		token_arctangent,
		token_arccotangent,
		token_arcsecant,
		token_arccosecant,
		token_common_logarithm,
		token_natural_logarithm,
		token_square_root,
		token_cubic_root,
		token_factorial,
		token_modulo,
		token_degree,
		token_radian
	};

	class number_token :public basic_token {
	private:
		double _value;
	public:
		number_token(double value) :basic_token(token_number), _value(value) {}
		double value()const { return _value; }
		double& rvalue() { return _value; }
	};

	class operator_token :public basic_token {
	protected:
		const std::string _str;
		const byte _operand_num;
		const byte _priority;
	public:
		operator_token(token_t type, const std::string& str, byte operand_num, byte priority)
			:basic_token(type), _str(str), _operand_num(operand_num), _priority(priority) {}
		virtual ~operator_token() {};
		const std::string& str()const { return _str; }
		byte operand_num()const { return _operand_num; }
		byte priority()const { return _priority; }
		virtual double apply(double left, double right)const = 0;
	};

	constexpr byte priority_max = 255;
	#define __create_operator_token(name, str, operand_num, priority, realise) \
	class name ## _token :public operator_token{ \
	public: \
		name ## _token() :operator_token(token_ ## name, str, operand_num, priority) {} \
		double apply(double left, double right)const { realise } \
	}
	__create_operator_token(plus, "+", 2, 1, return left + right;);
	__create_operator_token(minus, "-", 2, 1, return left - right;);
	__create_operator_token(modulo, "%", 2, 2, return fmod(left, right););
	__create_operator_token(multiply, "*", 2, 3, return left * right;);
	__create_operator_token(divide, "/", 2, 3, return left / right;);
	__create_operator_token(posite, "pos", 1, 4, return left;);
	__create_operator_token(negate, "neg", 1, 4, return -left;);
	__create_operator_token(exponent, "^", 2, 5, return pow(left, right););
	__create_operator_token(left_parentheses, "(", 0, 0, return 0;);
	__create_operator_token(right_parentheses, ")", 0, 0, return 0;);
	__create_operator_token(factorial, "!", 1, 6, return tgamma(left + 1););
	__create_operator_token(sine, "sin", 1, priority_max, return sin(left););
	__create_operator_token(cosine, "cos", 1, priority_max, return cos(left););
	__create_operator_token(tangent, "tan", 1, priority_max, return tan(left););
	__create_operator_token(cotangent, "cot", 1, priority_max, return 1.0 / tan(left););
	__create_operator_token(secant, "sec", 1, priority_max, return 1.0 / cos(left););
	__create_operator_token(cosecant, "csc", 1, priority_max, return 1.0 / sin(left););
	__create_operator_token(arcsine, "arcsin", 1, priority_max, return asin(left););
	__create_operator_token(arccosine, "arccos", 1, priority_max, return acos(left););
	__create_operator_token(arctangent, "arctan", 1, priority_max, return atan(left););
	__create_operator_token(arccotangent, "arccot", 1, priority_max, return atan(1.0 / left););
	__create_operator_token(arcsecant, "arcsec", 1, priority_max, return acos(1.0 / left););
	__create_operator_token(arccosecant, "arccsc", 1, priority_max, return asin(1.0 / left););
	__create_operator_token(common_logarithm, "lg", 1, priority_max, return log10(left););
	__create_operator_token(natural_logarithm, "ln", 1, priority_max, return log(left););
	__create_operator_token(square_root, "sqrt", 1, priority_max, return sqrt(left););
	__create_operator_token(cubic_root, "cbrt", 1, priority_max, return cbrt(left););
	__create_operator_token(degree, "deg", 1, priority_max, return left / pi * 180;);
	__create_operator_token(radian, "rad", 1, priority_max, return left / 180 * pi;);

	token_t string_to_operator_token_type(const std::string& operator_str);
	operator_token* token_type_to_operator_token(token_t operator_type);
}
#endif
