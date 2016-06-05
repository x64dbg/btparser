#ifndef DEF_OP_TRIPLE
#define DEF_OP_TRIPLE(enumval, ch1, ch2, ch3)
#endif //DEF_OP_TRIPLE
#ifndef DEF_OP_DOUBLE
#define DEF_OP_DOUBLE(enumval, ch1, ch2)
#endif //DEF_OP_DOUBLE
#ifndef DEF_OP_SINGLE
#define DEF_OP_SINGLE(enumval, ch1)
#endif //DEF_OP_SINGLE

DEF_OP_TRIPLE(tok_assign_shl, '<', '<', '=')
DEF_OP_TRIPLE(tok_assign_shr, '>', '>', '=')

DEF_OP_DOUBLE(tok_op_inc, '+', '+')
DEF_OP_DOUBLE(tok_op_dec, '-', '-')
DEF_OP_DOUBLE(tok_op_shl, '<', '<')
DEF_OP_DOUBLE(tok_op_shr, '>', '>')

DEF_OP_DOUBLE(tok_lop_less_equal, '<', '=')
DEF_OP_DOUBLE(tok_lop_greater_equal, '>', '=')
DEF_OP_DOUBLE(tok_lop_equal, '=', '=')
DEF_OP_DOUBLE(tok_lop_not_equal, '!', '=')
DEF_OP_DOUBLE(tok_lop_and, '&', '&')
DEF_OP_DOUBLE(tok_lop_or, '|', '|')

DEF_OP_DOUBLE(tok_assign_plus, '+', '=')
DEF_OP_DOUBLE(tok_assign_min, '-', '=')
DEF_OP_DOUBLE(tok_assign_mul, '*', '=')
DEF_OP_DOUBLE(tok_assign_div, '/', '=')
DEF_OP_DOUBLE(tok_assign_mod, '%', '=')
DEF_OP_DOUBLE(tok_assign_and, '&', '=')
DEF_OP_DOUBLE(tok_assign_xor, '^', '=')
DEF_OP_DOUBLE(tok_assign_or, '|', '=')

DEF_OP_SINGLE(tok_parenopen, '(')
DEF_OP_SINGLE(tok_parenclose, ')')
DEF_OP_SINGLE(tok_blockopen, '{')
DEF_OP_SINGLE(tok_blockclose, '}')
DEF_OP_SINGLE(tok_annotopen, '<')
DEF_OP_SINGLE(tok_annotclose, '>')
DEF_OP_SINGLE(tok_subopen, '[')
DEF_OP_SINGLE(tok_subclose, ']')
DEF_OP_SINGLE(tok_memberaccess, '.')
DEF_OP_SINGLE(tok_comma, ',')
DEF_OP_SINGLE(tok_tenary1, '?')
DEF_OP_SINGLE(tok_tenary2, ':')
DEF_OP_SINGLE(tok_assign, '=')

DEF_OP_SINGLE(tok_op_mul, '*')
DEF_OP_SINGLE(tok_op_div, '/')
DEF_OP_SINGLE(tok_op_mod, '%')
DEF_OP_SINGLE(tok_op_plus, '+')
DEF_OP_SINGLE(tok_op_min, '-')
DEF_OP_SINGLE(tok_op_neg, '~')
DEF_OP_SINGLE(tok_op_xor, '^')
DEF_OP_SINGLE(tok_op_and, '&')
DEF_OP_SINGLE(tok_op_or, '|')

DEF_OP_SINGLE(tok_lop_less, '<')
DEF_OP_SINGLE(tok_lop_greater, '>')
DEF_OP_SINGLE(tok_lop_not, '!')


