#define STATIC_ARRAY_SIZE(A) (sizeof(A)/sizeof(0[A]))

typedef signed __int8  s8;
typedef signed __int16 s16;
typedef signed __int32 s32;
typedef signed __int64 s64;

typedef unsigned __int8  u8;
typedef unsigned __int16 u16;
typedef unsigned __int32 u32;
typedef unsigned __int64 u64;

typedef s64 smm;
typedef u64 umm;

typedef s64 sint;
typedef u64 uint;

typedef u8 bool;
#define true 1
#define false 0

bool
Char_IsAlpha(u8 c)
{
	return (u8)((c&0xDF)-'A') < (u8)('Z' - 'A');
}

bool
Char_IsHexAlpha(u8 c)
{
	return (u8)((c&0xDF)-'A') < (u8)('F' - 'A');
}

bool
Char_IsDigit(u8 c)
{
	return (u8)(c - '0') < (u8)10;
}

typedef struct String
{
	u8* data;
	umm size;
} String;

#define STRING(S) (String){ .data = (u8*)(S), .size = sizeof(S) - 1 }
// NOTE: only necessary because Microsoft is bad at constants
#define MFSTRING(S) { .data = (u8*)(S), .size = sizeof(S) - 1 }

bool
String_Match(String s0, String s1)
{
	bool result = (s0.size == s1.size);

	for (umm i = 0; i < s0.size && result; ++i) result = (s0.data[i] == s1.data[i]);

	return result;
}

String
String_EatN(String s, umm n)
{
	return (n >= s.size ? (String){0} : (String){ .data = s.data + n, .size = s.size - n });
}

String
String_ChopN(String s, umm n)
{
	return (n >= s.size ? (String){0} : (String){ .data = s.data, .size = s.size - n });
}

typedef struct String_Builder
{
	u8* data;
	umm size;
	umm capacity;
	bool is_clipped;
} String_Builder;

bool
StringBuilder_PushChar(String_Builder* builder, u8 c)
{
	if (builder->size == builder->capacity) builder->is_clipped = true;
	else                                    builder->data[builder->size++] = c;

	return !builder->is_clipped;
}

bool
StringBuilder_PushString(String_Builder* builder, String string)
{
	if (string.size > builder->capacity || builder->size > builder->capacity - string.size)
	{
		builder->size       = builder->capacity;
		builder->is_clipped = true;
	}
	else
	{
		for (umm i = 0; i < string.size; ++i) builder->data[builder->size + i] = string.data[i];
		builder->size += string.size;
	}

	return !builder->is_clipped;
}

// NOTE: Markup Syntax
// *italic*
// **bold**
// ***bold italic***
// __underline__
// ~~strikethrough~~
// # H1
// ## H2
// ### H3
// #### H4
// `inline raw`
// ```c code```
// [link name](https://google.com)
// TODO:
// math
// tables

String CodeKeywords[] = {
	MFSTRING("if"),
	MFSTRING("else"),
	MFSTRING("return"),
	MFSTRING("true"),
	MFSTRING("false"),

	MFSTRING("s8"),
	MFSTRING("s16"),
	MFSTRING("s32"),
	MFSTRING("s64"),
	MFSTRING("u8"),
	MFSTRING("u16"),
	MFSTRING("u32"),
	MFSTRING("u64"),
	MFSTRING("smm"),
	MFSTRING("umm"),
	MFSTRING("sint"),
	MFSTRING("uint"),
	MFSTRING("bool"),
};

void
EncodeAndPushChar(String_Builder* builder, u8 c)
{
	if      (c == '"')  StringBuilder_PushString(builder, STRING("&quot;"));
	else if (c == '\'') StringBuilder_PushString(builder, STRING("&apos;"));
	else if (c == '&')  StringBuilder_PushString(builder, STRING("&amp;"));
	else if (c == '<')  StringBuilder_PushString(builder, STRING("&lt;"));
	else if (c == '>')  StringBuilder_PushString(builder, STRING("&gt;"));
	else                StringBuilder_PushChar(builder, c);
}

void
EncodeAndPushString(String_Builder* builder, String s)
{
	for (umm i = 0; i < s.size; ++i) EncodeAndPushChar(builder, s.data[i]);
}

bool
MarkupToHTML(String in, String_Builder* out)
{
	union
	{
		struct {
			u8 bold_level;
			bool is_underline;
			bool is_strikethrough;
			u8 heading_level;
		};
		u64 style_val;
	} style = {0};

	while (in.size != 0)
	{
		if (in.data[0] == '*')
		{
			u8 bold_level = 1;
			if (in.size > 1 && in.data[1] == '*') bold_level = 2;
			if (in.size > 2 && in.data[2] == '*') bold_level = 3;

			in = String_EatN(in, bold_level);

			if (style.bold_level == 0)
			{
				style.bold_level = bold_level;

				StringBuilder_PushString(out, (bold_level == 1 ? STRING("<em class=\"italic\">")
							                        :bold_level == 2 ? STRING("<b class=\"bold\">")
																			:                  STRING("<b class=\"bold_italic\">")));
			}
			else
			{
				if (style.bold_level != bold_level)
				{
					//// ERROR: previous level must be closed before opening another
					return false;
				}
				else
				{
					style.bold_level = 0;

					StringBuilder_PushString(out, (bold_level == 1 ? STRING("</em>") : STRING("</b>")));
				}
			}
		}
		else if (in.size > 1 && in.data[0] == '_' && in.data[1] == '_')
		{
			in = String_EatN(in, 2);

			StringBuilder_PushString(out, (style.is_underline ? STRING("</u>") : STRING("<u>")));
			style.is_underline = !style.is_underline;
		}
		else if (in.size > 1 && in.data[0] == '~' && in.data[1] == '~')
		{
			in = String_EatN(in, 2);

			StringBuilder_PushString(out, (style.is_strikethrough ? STRING("</s>") : STRING("<s>")));
			style.is_strikethrough = !style.is_strikethrough;
		}
		else if (in.data[0] == '#')
		{
			if (style.heading_level != 0)
			{
				//// ERROR: headings cannot be nested
				return false;
			}
			else
			{
				uint heading_level = 0;
				while (in.size > heading_level && in.data[heading_level] == '#') ++heading_level;

				if (heading_level > 6)
				{
					//// ERROR: heading level too high
					return false;
				}
				else if (in.size < heading_level + 1 || in.data[heading_level] != ' ')
				{
					//// ERROR: missing space after heading marker
					return false;
				}
				else
				{
					in = String_EatN(in, heading_level + 1);

					style.heading_level = (u8)heading_level;

					StringBuilder_PushString(out, STRING("<h"));
					StringBuilder_PushChar(out, '0' + style.heading_level);
					StringBuilder_PushString(out, STRING(">"));
				}
			}
		}
		else if (in.data[0] == '´')
		{
			if (style.style_val != 0)
			{
				//// ERROR: unterminated style
				return false;
			}
			else
			{
				if (!(in.size > 2 && in.data[1] == '´' && in.data[2] == '´'))
				{
					String raw_text = { .data = in.data + 1 };

					in = String_EatN(in, 1);
					while (in.size != 0 && in.data[0] != '`') in = String_EatN(in, 1);

					if (in.size == 0)
					{
						//// ERROR: Missing terminating backtick after raw block
						return false;
					}
					else
					{
						in = String_EatN(in, 1);

						StringBuilder_PushString(out, STRING("<span class=\"raw_text\">"));
						EncodeAndPushString(out, raw_text);
						StringBuilder_PushString(out, STRING("</span>"));
					}
				}
				else
				{
					StringBuilder_PushString(out, STRING("<code>"));

					in = String_EatN(in, 3);

					for (;;)
					{
						if (in.size < 3)
						{
							//// ERROR: Missing terminating triple backtick after code block
						}
						else if (in.data[0] == '`' && in.data[1] == '`' && in.data[2] == '`')
						{
							in = String_EatN(in, 3);
							StringBuilder_PushString(out, STRING("</code>"));
							break;
						}
						else
						{
							if (Char_IsAlpha(in.data[0]))
							{
								String identifier = { .data = in.data };
								while (in.size != 0 && Char_IsAlpha(in.data[0])) in = String_EatN(in, 1);

								identifier.size = in.data - identifier.data;

								bool is_keyword = false;
								for (uint i = 0; i < STATIC_ARRAY_SIZE(CodeKeywords); ++i)
								{
									if (String_Match(identifier, CodeKeywords[i]))
									{
										is_keyword = true;
										break;
									}
								}

								if (is_keyword)
								{
									StringBuilder_PushString(out, STRING("<span class=\"code_keyword\">"));
									StringBuilder_PushString(out, identifier);
									StringBuilder_PushString(out, STRING("</span>"));
								}
								else StringBuilder_PushString(out, identifier);
							}
							else if (in.data[0] == '"')
							{
								String string_lit = { .data = in.data };

								in = String_EatN(in, 1);
								while (in.size != 0 && in.data[0] != '"')
								{
									if (in.data[0] == '\\')
									{
										if (in.size > 2) in = String_EatN(in, 2);
										else
										{
											//// ERROR: Missing escape sequence after backslash in string literal
											return false;
										}
									}
									else in = String_EatN(in, 1);
								}

								if (in.size == 0)
								{
									//// ERROR: Missing terminating quote after string literal
									return false;
								}
								else
								{
									in = String_EatN(in, 1);

									StringBuilder_PushString(out, STRING("<span class=\"code_string_lit\">"));
									EncodeAndPushString(out, string_lit);
									StringBuilder_PushString(out, STRING("</span>"));
								}
							}
							else if (Char_IsDigit(in.data[0]))
							{
								String number = { .data = in.data };

								bool is_hex = (in.size > 1 && in.data[0] == 0 && (in.data[1]&0xDF) == 'X');

								while (in.size != 0 && (Char_IsDigit(in.data[0]) || is_hex && Char_IsHexAlpha(in.data[0]))) in = String_EatN(in, 1);

								if (!is_hex && in.size != 0 && in.data[0] == '.')
								{
									in = String_EatN(in, 1);

									if (in.size == 0)
									{
										//// ERROR: Missing decimals of float literal
										return false;
									}
									else
									{
										while (in.size != 0 && Char_IsDigit(in.data[0])) in = String_EatN(in, 1);

										if (in.size != 0 && (in.data[0]&0xDF) == 'E')
										{
											in = String_EatN(in, 1 + (in.size != 0 && (in.data[0] == '+' || in.data[0] == '-')));

											if (in.size == 0)
											{
												//// ERROR: Missing digits of float exponent
												return false;
											}
											else while (in.size != 0 && Char_IsDigit(in.data[0])) in = String_EatN(in, 1);
										}

										if (in.size != 0 && (in.data[0]&0xDF) == 'F') in = String_EatN(in, 1);
									}
								}

								number.size = in.data - number.data;

								StringBuilder_PushString(out, STRING("<span class=\"code_number\">"));
								StringBuilder_PushString(out, number);
								StringBuilder_PushString(out, STRING("</span>"));
							}
						}
					}
				}
			}
		}
		else if (in.data[0] == '[')
		{
			if (style.style_val != 0)
			{
				//// ERROR: links cannot be styled
				return false;
			}
			else
			{
				String link_text = { .data = in.data + 1 };
				while (in.size != 0 && in.data[0] == ']') in = String_EatN(in, 1);

				if (in.size == 0)
				{
					//// ERROR: Missing terminating ] after link text
				}
				else
				{
					link_text.size = in.data - link_text.data;
					in = String_EatN(in, 1);

					if (link_text.size == 0)
					{
						//// ERROR: Link must have link text to display
						return false;
					}
					else if (in.size == 0 || in.data[0] != '(')
					{
						//// ERROR: Missing open parenthesis after link text
						return false;
					}
					else
					{
						String link_dest = { .data = in.data + 1 };

						while (in.size != 0 && in.data[0] != ')') in = String_EatN(in, 1);

						link_dest.size = in.data - link_dest.data;

						if (in.size == 0)
						{
							//// ERROR: Missing terminating close parenthesis after link destination
							return false;
						}
						else if (link_dest.size == 0)
						{
							//// ERROR: Link must have a valid destination url
							return false;
						}
						else
						{
							in = String_EatN(in, 1);

							StringBuilder_PushString(out, STRING("<a href=\""));
							EncodeAndPushString(out, link_dest);
							StringBuilder_PushString(out, STRING("\">"));
							EncodeAndPushString(out, link_text);
							StringBuilder_PushString(out, STRING("</a>"));
						}
					}
				}
			}
		}
		else
		{
			if (in.data[0] == '\n')
			{
				if (style.heading_level != 0)
				{
					style.heading_level = 0;
					StringBuilder_PushString(out, STRING("</h"));
					StringBuilder_PushChar(out, '0' + style.heading_level);
					StringBuilder_PushString(out, STRING(">"));
				}

				StringBuilder_PushChar(out, '\n');
				in = String_EatN(in, 1);
			}
			else if (in.data[0] == '\\')
			{
				if (in.size < 2)
				{
					//// ERROR: Missing character to escape after backslash
					return false;
				}
				else
				{
					EncodeAndPushChar(out, in.data[1]);

					in = String_EatN(in, 2);
				}
			}
			else
			{
				EncodeAndPushChar(out, in.data[0]);
				in = String_EatN(in, 1);
			}
		}
	}

	return true;
}

int
main(int argc, char** argv)
{
	(void)argc;
	(void)argv;

	return 0;
}
