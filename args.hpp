// MIT License
// 
// Copyright (c) 2025 Enrique Rocha Benatti <rochabenattienrique@gmail.com>
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once

#include <map>
#include <print>
#include <string>
#include <format>
#include <variant>
#include <sstream>
#include <algorithm>
#include <functional>
#include <typeindex>

namespace args {

#define ARGS_TYPE(T) std::optional<T>

#define ARGS_DEFAULT_TYPES						\
	    std::optional<long>,					\
		std::optional<double>,					\
		std::optional<std::string>,				\
		std::optional<std::vector<std::string>>

#ifdef ARGS_CUSTOM_TYPES
	using opt_var_arg = std::variant<ARGS_DEFAULT_TYPES, ARGS_CUSTOM_TYPES>;
#else
	using opt_var_arg = std::variant<ARGS_DEFAULT_TYPES>;
#endif


	template<typename T> static T parse_value(const std::string& str, char seperator);
	
#define ARGS_PARSE_FUNC(Type, Body)										\
	template<> inline Type parse_value(const std::string& args_str, char args_delim) Body
#define ARGS_PARSE_EXPR(Type, expr) ARGS_PARSE_FUNC(Type, { return expr; })

	ARGS_PARSE_EXPR(long, std::stol(args_str))
	ARGS_PARSE_EXPR(bool, args_str.size() == 0)
	ARGS_PARSE_EXPR(double, std::stod(args_str))
	ARGS_PARSE_EXPR(std::string, args_str)
		
	ARGS_PARSE_FUNC(std::vector<std::string>,
					{
						std::vector<std::string> str_vec;
						auto ss = std::istringstream(args_str);
						for (std::string line; std::getline(ss, line, args_delim);)
							str_vec.emplace_back(line);
						return str_vec;
					})
	
	struct Argument {
		std::string flag;
		std::string help_str;
		opt_var_arg value;

		Argument(std::string flag, std::string help_str, opt_var_arg value)
			: flag(std::move(flag)),
			  help_str(std::move(help_str)),
			  value(std::move(value))
		{}

		void emplace_parsed(const std::string& str, char delim) {
			std::visit([&](auto& opt_val){
				using T = typename std::decay_t<decltype(opt_val)>::value_type;
				opt_val = parse_value<T>(str, delim);
			}, value);
		}
	};

	class Cli_Parser {
	public:
		Cli_Parser(bool orderMaters, char delim = ' ')
			: delim(delim), m_OrderMaters(orderMaters) {}

		char delim;
	
		template<typename T>
		void add(const std::string& flag, const std::string& help_str = "", opt_var_arg value = T(0)) {
			m_ArgsMap.emplace(flag, Argument{flag, help_str, value});
		}

		bool parse(int argc, char **argv) {
			bool resul =  m_OrderMaters ? ordered_parse(argc, argv) : unordered_parse(argc, argv);
			return resul;
		}

		template<typename T>
		T& get(const std::string& key) {
			return *std::get<std::optional<T>>(m_ArgsMap.at(key).value);
		}

		const std::string& error_str() const { return m_ErrorMsg; }
	
		std::string usage_str(const char *program_name) const {
			auto usage_str = std::format("Usage: {} ", program_name);
			if (m_ArgsMap.size() == 0) return usage_str;
			std::size_t max_flag_str_size = 0;
		
			for (const auto& [flag, arg] : m_ArgsMap) {
				usage_str.append(std::format("-{} ", flag));

				if (is_type<std::vector<std::string>>(arg.value)) 
					usage_str.append(std::format("<{}_0> <{}_1> ... ", flag, flag));
				else
					usage_str.append(std::format("<{}> ", flag));

				if (flag.size() > max_flag_str_size)
					max_flag_str_size = flag.size();
			}
		
			usage_str.append({'\n'});
		
			for (const auto& [flag, arg] : m_ArgsMap) {
				usage_str.append(std::format("\t{0:>{1}}\t", flag, max_flag_str_size));

				if (arg.help_str.size() > 0) 
					usage_str.append(arg.help_str);
			
				usage_str.append({'\n'});
			}

			return usage_str;
		}

		std::string debug_str() {
			auto sb = std::string("class Args (debug):");
		
			for (const auto& [flag, arg] : m_ArgsMap) {
				sb.append(std::format("\tflag: {}", flag));
			
				bool visited = std::visit([&sb](auto& opt_val) -> bool {
					using opt_t = typename std::decay_t<decltype(opt_val)>::value_type;
					sb.append(std::format("\t\ttype: {}\n", std::type_index(typeid(opt_t)).name()));
				
					if (opt_val) sb.append(std::format("\t\tvalue: {}\n", *opt_val));
					else         sb.append(std::format("\t\tvalue: null\n"));

					return true;
				}, arg.value);
			
				if (!visited) sb.append("unable to visit");
			}
		
			return sb;
		}
	
	private:
		std::map<std::string, Argument> m_ArgsMap;
		std::string m_ErrorMsg;
		bool m_OrderMaters;

		template<typename T>
		constexpr bool is_type(opt_var_arg v) const { return std::holds_alternative<std::optional<T>>(v); }

		bool check_still_left(int index, int argc, const std::string& key) {
			if (index >= argc) {
				m_ErrorMsg = std::format("Expected a value to key '{}' at the end", key);
				return false;
			}
			return true;
		}

		void set_argument_value(const std::string& key, int& i, int argc, char **argv) {
			auto& argument = m_ArgsMap.at(key);
			std::string arg_str;

			// collect to the str
			for (; i < argc && *argv[i] != '-'; i++) {
				arg_str.append(argv[i]);
				arg_str.append({delim});
			}
			i--;
			
			argument.emplace_parsed(arg_str, delim);
			i++;
		}

		bool unordered_parse(int argc, char **argv) {
			for (int i = 1; i < argc; i++) {
				std::string str = std::string(argv[i]);
				if (str.starts_with('-')) {
					std::string key = str.substr(1);
					i++;

					if (!m_ArgsMap.contains(key)) {
						m_ErrorMsg = std::format("Did NOT expect key '{0}'. forgot to call .add(\"{0}\")?", key);
						return false;
					}

					if (!check_still_left(i, argc, key)) return false;

					set_argument_value(key, i, argc, argv);
				} 
			}

			return true;
		}

		bool ordered_parse(int argc, char **argv) {
			int i = 1;
			for (auto& [key, value] : m_ArgsMap) {
				if (!check_still_left(i, argc, key)) return false;
			
				std::string str = std::string(argv[i]);
				if (!str.starts_with('-')) {
					m_ErrorMsg = std::format("Expected '{}' to be a flag (starts with '-')", str);
					return false;
				}
				
				std::string argvKey = str.substr(1);
				if (argvKey != key) {
					m_ErrorMsg = std::format("Expected '{}' to be a key for key '{}'", argvKey, key);
					return false;
				}

				i++;

				set_argument_value(key, i, argc, argv);
			}

			return true;
		}
	};
}
