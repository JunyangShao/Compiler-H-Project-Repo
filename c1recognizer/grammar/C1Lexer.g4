lexer grammar C1Lexer;

tokens {
    Comma,
    SemiColon,
    Assign,
    LeftBracket,
    RightBracket,
    LeftBrace,
    RightBrace,
    LeftParen,
    RightParen,
    If,
    Else,
    While,
    Const,
    Equal,
    NonEqual,
    Less,
    Greater,
    LessEqual,
    GreaterEqual,
    Plus,
    Minus,
    Multiply,
    Divide,
    Modulo,

    Int,
    Float,
    Void,

    Identifier,
    IntConst,
    FloatConst
}

Comma: ',';
SemiColon: ';';
Assign: '=';
LeftBracket: '[';
RightBracket: ']';
LeftBrace: '{';
RightBrace: '}';
LeftParen: '(';
RightParen: ')';
If: 'if';
Else: 'else';
While: 'while';
Const: 'const';
Equal: '==';
NonEqual: '!=';
Less: '<';
Greater: '>';
LessEqual: '<=';
GreaterEqual: '>=';
Plus: '+';
Minus: '-';
Multiply: '*';
Divide: '/';
Modulo: '%';

Int: 'int';
Float: 'float';
Void: 'void';

fragment Decimal_constant: '0' | [1-9]([0-9]+)?;
fragment Octal_constant: '0'[0-7]+;
fragment Hexadecimal_constant: ('0x'|'0X')[0-9a-fA-F]+;
IntConst: Decimal_constant | Octal_constant | Hexadecimal_constant;

fragment Digit_sequence: [0-9]+;
fragment Sign: '+' | '-' ;
fragment Exponent_part: 'e'Sign?Digit_sequence | 'E'Sign?Digit_sequence;
fragment Fractional_constant: (Digit_sequence)? '.' Digit_sequence | Digit_sequence '.' ;
fragment Decimal_floating_constant: Fractional_constant Exponent_part? | Digit_sequence Exponent_part;
FloatConst: Decimal_floating_constant;

Identifier: [a-zA-Z_]([a-zA-Z0-9_]+)?;

WhiteSpace: [ \t\r\n]+ -> skip;

fragment Other: ~'*';
fragment Other1: ~('*' | '/' );
Comment: '/*'(Other+)?(('*'+Other1 (Other+)?)+)?'*'+'/' -> skip;
fragment NonNextline: ~'\n';
Comment2: ('/''\\''\n''/' | '//' )(('\\''\n' | NonNextline)+)? ->skip;