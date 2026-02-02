Ini adalah file dimana anda dapat melihat progress saat ini tentang bahasa pemrograman sonic yang sedang dikembangkan.

Dalam pengembangan bahasa pemrograman, terdapat beberapa langkah yang perlu di lakukan dengan baik.

- Lexical Analysis
- Parser Analysis & AST Builder
- Semantic Analysis
- Code Generation

4 tahap diatas hanyalah permulaan, jika membuat bahasa yang serius tentu masih ada tahap lainnya.

---
### Lexical Analysis
- [x] Read String
- [x] Read Char
- [x] Read Number
- [x] Read Identifier & Keyword
- [x] Read Punctuation
### Parser Analysis
- [x] Extern Declaration
- [ ] Parse Import Statement
- [x] Parse Function Declaration
- [x] Parse Variable Declaration
- [x] Parse Type
	- [x] Primitive (i32, i64, i128, f32, f64, bool, string, char)
	- [ ] Scope Access
	- [x] Identifier
	- [x] Reference & Pointer
- [x] Parse Expression
	- [x] Pointer & Reference
	- [x] Unary Expr
	- [x] Binary Expr
	- [x] String
	- [x] Number
	- [x] Character
	- [x] None
	- [x] Boolean
	- [x] Identifier
		- [x] Variable
		- [x] Call
		- [x] Index Access
		- [x] Scope Access
		- [x] Member Access
- [ ] Parse Conditional Statement
- [ ] Parse For Loop Statement
- [ ] Parse While Loop Statement
- [ ] Parse Try Catch Statement
- [ ] Parse Return Statement
- [ ] Parse Switch Case Statement
- [x] Parse Assignment & Call Expr
