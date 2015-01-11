/*
 * Copyright (c) 2013-2014, Roland Bock
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 * 
 *   Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 * 
 *   Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef SQLPP_UNION_H
#define SQLPP_UNION_H

#include <sqlpp11/statement_fwd.h>
#include <sqlpp11/type_traits.h>
#include <sqlpp11/parameter_list.h>
#include <sqlpp11/expression.h>
#include <sqlpp11/interpret_tuple.h>
#include <sqlpp11/interpretable_list.h>
#include <sqlpp11/logic.h>

namespace sqlpp
{
	template<typename Statement, typename Enable = void>
		struct has_result_row_impl
		{
			using type = std::false_type;
		};

	template<typename Statement>
		struct has_result_row_impl<Statement, 
		typename std::enable_if<
			not wrong_t<typename Statement::template _result_methods_t<Statement>::template _result_row_t<void>>::value, 
		void>::type>
		{
			using type = std::true_type;
		};

	template<typename Statement>
		using has_result_row_t = typename has_result_row_impl<Statement>::type;

	template<typename Statement, typename Enable = void>
		struct get_result_row_impl
		{
			using type = void;
		};

	template<typename Statement>
		struct get_result_row_impl<Statement, 
		typename std::enable_if<
			not wrong_t<typename Statement::template _result_methods_t<Statement>::template _result_row_t<void>>::value, 
		void>::type>
		{
			using type = typename Statement::template _result_methods_t<Statement>::template _result_row_t<void>;
		};

	template<typename Statement>
		using get_result_row_t = typename get_result_row_impl<Statement>::type;


	struct no_union_t;

	using blank_union_t = statement_t<void,
				no_union_t>;
	// There is no order by or limit or offset in union, use it as a pseudo table to do that.
	
	template<bool, typename Union>
		struct union_statement_impl
		{
			using type = statement_t<void, Union>;
		};

	template<typename Union>
		struct union_statement_impl<false, Union>
		{
			using type = bad_statement;
		};

	template<bool Check, typename Union>
		using union_statement_t = typename union_statement_impl<Check, Union>::type;


	// UNION DATA
	template<typename Database, typename Flag, typename Lhs, typename Rhs>
		struct union_data_t
		{
			union_data_t(Lhs lhs, Rhs rhs):
				_lhs(lhs),
				_rhs(rhs)
			{}

			union_data_t(const union_data_t&) = default;
			union_data_t(union_data_t&&) = default;
			union_data_t& operator=(const union_data_t&) = default;
			union_data_t& operator=(union_data_t&&) = default;
			~union_data_t() = default;

			Lhs _lhs;
			Rhs _rhs;
		};

	// UNION(EXPR)
	template<typename Database, typename Flag, typename Lhs, typename Rhs>
		struct union_t
		{
			using _traits = make_traits<no_value_t, tag::is_union, tag::is_select_column_list, tag::is_return_value>;
			using _recursive_traits = make_recursive_traits<Lhs, Rhs>;

			using _alias_t = struct{};

			// Data
			using _data_t = union_data_t<Database, Flag, Lhs, Rhs>;

			// Member implementation with data and methods
			template <typename Policies>
				struct _impl_t
				{
				public:
					_data_t _data;
				};

			// Base template to be inherited by the statement
			template<typename Policies>
				struct _base_t
				{
					using _data_t = union_data_t<Database, Flag, Lhs, Rhs>;

					_impl_t<Policies> union_;
					_impl_t<Policies>& operator()() { return union_; }
					const _impl_t<Policies>& operator()() const { return union_; }

					using _selected_columns_t = decltype(union_._data._lhs.selected_columns);
					_selected_columns_t& get_selected_columns() { return union_._data._lhs.selected_columns;}
					const _selected_columns_t& get_selected_columns() const { return union_._data._lhs.selected_columns; }

					template<typename T>
						static auto _get_member(T t) -> decltype(t.union_)
						{
							return t.union_;
						}

					using _consistency_check = consistent_t;
				};

			template<typename Statement>
			using _result_methods_t = typename Lhs::template _result_methods_t<Statement>;
		};

	// NO UNION YET
	struct no_union_t
	{
		using _traits = make_traits<no_value_t, tag::is_union>;
		using _recursive_traits = make_recursive_traits<>;

		// Data
		using _data_t = no_data_t;

		// Member implementation with data and methods
		template<typename Policies>
			struct _impl_t
			{
				_data_t _data;
			};

		// Base template to be inherited by the statement
		template<typename Policies>
			struct _base_t
			{
				using _data_t = no_data_t;

				_impl_t<Policies> no_union;
				_impl_t<Policies>& operator()() { return no_union; }
				const _impl_t<Policies>& operator()() const { return no_union; }

				template<typename T>
					static auto _get_member(T t) -> decltype(t.no_union)
					{
						return t.no_union;
					}

				using _database_t = typename Policies::_database_t;

				template<typename... T>
					using _check = logic::all_t<is_statement_t<T>::value...>; // FIXME and consistent/runnable

				template<typename Check, typename T>
					using _new_statement_t = union_statement_t<Check::value, T>;

				using _consistency_check = consistent_t;

				template<typename Rhs>
					auto union_distinct(Rhs rhs) const
					-> _new_statement_t<_check<Rhs>, union_t<void, distinct_t, derived_statement_t<Policies>, Rhs>>
					{
						static_assert(is_statement_t<Rhs>::value, "argument of union call has to be a statement");
						static_assert(has_result_row_t<Rhs>::value, "argument of a union has to be a (complete) select statement");
						static_assert(has_result_row_t<derived_statement_t<Policies>>::value, "left hand side argument of a union has to be a (complete) select statement");
						static_assert(std::is_same<get_result_row_t<derived_statement_t<Policies>>, get_result_row_t<Rhs>>::value, "both select statements in a union have to have the same result columns (type and name)");

						return _union_impl<void, distinct_t>(_check<derived_statement_t<Policies>, Rhs>{}, rhs);
					}

				template<typename Rhs>
					auto union_all(Rhs rhs) const
					-> _new_statement_t<_check<Rhs>, union_t<void, all_t, derived_statement_t<Policies>, Rhs>>
					{
						static_assert(is_statement_t<Rhs>::value, "argument of union call has to be a statement");
						static_assert(has_result_row_t<Rhs>::value, "argument of a union has to be a (complete) select statement");
						static_assert(has_result_row_t<derived_statement_t<Policies>>::value, "left hand side argument of a union has to be a (complete) select statement");
						static_assert(std::is_same<get_result_row_t<derived_statement_t<Policies>>, get_result_row_t<Rhs>>::value, "both select statements in a union have to have the same result columns (type and name)");


						return _union_impl<void, all_t>(_check<derived_statement_t<Policies>, Rhs>{}, rhs);
					}

			private:
				template<typename Database, typename Flag, typename Rhs>
					auto _union_impl(const std::false_type&, Rhs rhs) const
					-> bad_statement;

				template<typename Database, typename Flag, typename Rhs>
					auto _union_impl(const std::true_type&, Rhs rhs) const
					-> _new_statement_t<std::true_type, union_t<Database, Flag, derived_statement_t<Policies>, Rhs>>
					{
						return { blank_union_t{}, 
							union_data_t<_database_t, Flag, derived_statement_t<Policies>, Rhs>{static_cast<const derived_statement_t<Policies>&>(*this), rhs} };
					}

			};
	};

	// Interpreters
	template<typename Context, typename Database, typename Flag, typename Lhs, typename Rhs>
		struct serializer_t<Context, union_data_t<Database, Flag, Lhs, Rhs>>
		{
			using _serialize_check = serialize_check_of<Context, Lhs, Rhs>;
			using T = union_data_t<Database, Flag, Lhs, Rhs>;

			static Context& _(const T& t, Context& context)
			{
				context << '(';
				serialize(t._lhs, context);
				context << ") UNION ";
				serialize(Flag{}, context);
				context << " (";
				serialize(t._rhs, context);
				context << ')';
				return context;
			}
		};

}

#endif
