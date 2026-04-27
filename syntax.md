# luaBase (lb) Language Specification v2.7.3
Please view this file using an actual markdown reader, not the default github markdown preview.
---
## 1. Installation
$$
\text{bash terminal} \to \text{install.sh}
\\
\text{exact command} \to \text{"./install.sh"}
$$
---
## 2. Core Data Types and Prints.
### Printing
$$
\text{print("hello world! your name is: ", name, "\text{\textbackslash}n")}\ \ \\//needs\ manual \ newline\ though.
\\
\text{println("hello world! you are: ", name)}\ \ \\//places\ newline\ automatically.
\\
printfmt("hello\ world!\ you\ are: \%s\text{\textbackslash}n",\ name)\ \ \\//direct\ c\ printf\ call.
$$
### Data Types
$$

\begin{array}{|l|l|l|}
\hline
\mathbf{Category} & \mathbf{luaBase\ Type} & \mathbf{C\ Representation} \\ \hline
\text{Integers} & \text{int, short, long} & \text{int, short, long} \\ \hline
\text{Fixed-Width} & \text{u8, u32, u64} & \text{uint8\_t, uint32\_t, uint64\_t} \\ \hline
\text{Floating Point} & \text{float, double} & \text{float, double} \\ \hline
\text{Text} & \text{str, char} & \text{char[256], char} \\ \hline
\text{Logic} & \text{bool} & \text{bool (true/false)} \\ \hline
\text{SIMD} & \text{\_\_m256, \_\_m256i} & \text{AVX Vector Intrinsics} \\ \hline
\text{Pointers} & \text{ptr } \langle type \rangle & \text{Type-safe pointers } \\ \hline
\end{array}
$$



## 3. Variables and Arrays
$$
\begin{array}{l}
\text{int count = 0} \\
\text{global u32 system\_addr = 0xFF00} \\
\text{const str VERSION = "2.7.3"} \\
\text{int scores[5] = \{10, 20, 30, 40, 50\}}
\end{array}
$$

---

## 4. Control Flow

### If-Then-Else
$$
\begin{array}{l}
\text{if } x > 10 \text{ then} \\
\quad \text{println("Greater")} \\
\text{elseif } x == 10 \text{ then} \\
\quad \text{println("Equal")} \\
\text{else} \\
\quad \text{println("Lesser")} \\
\text{end}
\end{array}
$$

### Loops
$$
\begin{array}{l}
\text{for } i = 0, 10, 1 \text{ do} \\
\quad \text{print(i)} \\
\text{end} \\ \\
\text{while } \text{not } \text{finished do} \\
\quad \text{process()} \\
\text{end}
\end{array}
$$

---

## 5. Functions
$$
\begin{array}{l}
\text{function int fib(int n) \{} \\
\quad \text{if n < 2 then return n end} \\
\quad \text{return fib(n-1) + fib(n-2)} \\
\text{\}} \\ \\
\text{inline function float square(float f) \{ return f * f \}}
\end{array}
$$

---

## 6. Memory Operations
$$
\begin{array}{l}
\text{ptr int p = \&x} \\
\text{*p = 100} \\
\text{memset(ptr, 0, size)} \\
\text{sizeof(u64)} \\
\text{cast(int, 3.14)} \\
\text{typeof(var)}
\end{array}
$$

---

## 7. Structures and Enums
$$
\begin{array}{l}
\text{type Player \{} \\
\quad \text{int id} \\
\quad \text{str name} \\
\text{\}} \\ \\
\text{enum State \{} \\
\quad \text{IDLE = 0, BUSY = 1} \\
\text{\}}
\end{array}
$$

---

## 8. Advanced Features
$$
\begin{array}{l}
\text{try \{} \\
\quad \text{if error then throw "Failure" end} \\
\text{\} except (str msg) \{} \\
\quad \text{println("Caught: ", msg)} \\
\text{\}} \\ \\
\text{link "math.lh"} \\
\text{link "stdio.h"}
\end{array}
$$

---

## 9. Compiler Flags (lcc)
$$
\begin{array}{|l|l|}
\hline
\text{Flag} & \text{Purpose} \\ \hline
\text{-c} & \text{Keep debug .lb.c file} \\ \hline
\text{-asm} & \text{Generate assembly code} \\ \hline
\text{-main} & \text{Skip top-level auto-main} \\ \hline
\text{-l[path]} & \text{Add include directory} \\ \hline
\text{-g[lib]} & \text{Link external library} \\ \hline
\text{-w[libpath]} & \text{Library Paths}
\\ \hline
\text{-debug} & \text{Skip Optimization Phase}
\\ \hline
\end{array}

$$
---
## 10. Uninstallation
$$
\text{easiest way:}
\\
bash \to install.sh
\\
exact\ command \to \text{"./install.sh"}
$$
---
## 11. Standard Library
### Accessing the standard library
---
$$
\text{Just do: link "library.lh"}\\
\text{Standard library is installed when you do ./install.sh}\\
\text{AUTOMATICALLY!!}
$$
---
### List of LH files in the standard library:
$$
\begin{array}{|l|l|}
\hline
\text{Name} & \text{Purpose} \\ \hline
\text{Math.lh} & \text{General purpose Math library} \\ \hline
\text{stdlib.lh} & \text{Provide general functions} \\ \hline
\text{MathGraphic.lh} & \text{Provide Graphical Math functions} \\ \hline
\end{array}

$$ 