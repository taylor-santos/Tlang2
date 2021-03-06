/*
Functor := <T>class {
    map : <S>func(fn: func(T) => S) => <S>self;
};

List := <T>class: <T>Functor{};

IntList := new <int>List();
half := func(i: int) => float {
    return i / 2.;
};
halfList := IntList.map(half);

comp := <A, B, C>func(
    f2: <B, C>func(B) => C,
    f1: <A, B>func(A) => B
) => func(A) => C {
  return func(a: A) => C {
    return f2(f1(a));
  };
};

Pair := <T, S>class {
    first: T;
    second: S;
};

a, b, c := (1, 2, 3);

text := "Hello, world!";

foo();

t: (int, int..5, string);

Foo := class {
    x: int;
    y: string;
};

impl Foo {
    Foo := func() => none {
        x := 0;
        y := "Foo";
    };
}

a, b, t := *(1, 2, (4, 5, 6));


list: []int;

A := class {
    a: int;
};

B := class {
    a: func() => int;
    b: func() => none;
};

C := class {
    a: int;
    b: int;
};

D := class {
    a: func() => int;
};

Foo := class {
    x : func(C) => D;
};
Bar := class {
    x : func(A) => B;
};
fn := func(foo: Foo) => int {
    return foo.x(new C()).a();
};

bar := new Bar();

a := fn(bar);
*/
/*
Foo := class {
    a: int;
    x: int;
    operator == (int) => bool;
    operator => (int);
};

Bar := class {
    a: int;
    y: int;
};

A := class {
    a: int;
};

fn := func(a: A) => int {
    return a.a;
};

fn2 := func() => int {
    x := 5;
    while false {
        return x;
    }
    y: int;
    do {
        y := 5;
    } while false;
    return y;
};

main := func(b: bool) => int {
    c := new Foo();
    switch c {
        case 5 {
        }
    }
    return 0;
    d := c => int;
};

aFoo := new Foo();

if aFoo => int {

}
*/

/*

foo := func(x: int) => bool {
    return x => bool;
};

foo := func(x: bool) => int {
    if x {
        return 5;
    } else {
        return 0;
    }
};

*/
/*
t := (1, 2, 3, 4, "test", 6, 7, 8, 9);


_, _, _, _, s, _, _, _, _ := *t;

A := class {
    x: int;
};

B := class {
    x: int;
    y: string;
};

C := class {
    y: string;
};

D := class {
    x: int;
    y: string;
};

foo := func(x: int) => bool {
    return x => bool;
};

foo := func(x: bool) => int {
    if x {
        return 5;
    } else {
        return 0;
    }
};
*/
/*
f := func(d: double) => none {};
x := 5;
g := func(a: int, b: int) => int { return a; };
if true {
    f := func(i: int) => int {
        return i;
    };
    f(x);
    (func(x: int) => func(int) => int {
        return func(y: int) => int {
            return g(x, y);
        };
    })(5);
}
*/

/*
A: class {
    x: int;
};

B: class {
    x: int;
    y: string;
};

C: class {
    y: string;
};

D: class {
    x: int;
    y: string;
};
*/
/*
A: class {
    x: int;
};

B: class {
    x: int;
};

foo := func(a: A) => none {};
foo := func(b: B) => none {};

a := new A();
foo(a);
*/
/*
B: class {
    x: int;
};
C: class {
    y: bool;
};
A: class {
    x: int;
    y: bool;
};
a = new A();
b = 5;
b += 10;
*/
/*
foo = func(x: int, y: double) => int {
    a = y*y + x=>double;
    bar = func(z: int) => int {
        return z;
    };
    return bar(a => int + x);
};
x = 6.4 => int;
y = foo(x, 3.2);
*/
/*
foo = func(x: ref int, y: int, fn: func(ref int, int) => none) => none {
    fn(ref x, y);
};

bar = func(x: ref int, y: int) => none {
    x = y-x;
};

a = 10;
foo(ref a, 5, bar);
*/
/*
y = 0;
foo = func(x: ref int) => none {
    x = y;
};

bar = func() => int {
    a = 10;
    foo(ref a);
    return a;
};
y = 10;
x = bar();
*/
a = 0;
foo = func() => none {};
bar = func() => none {
    a += 1;
};

baz = func(fn: ref func() => none) => none {
    fn = bar;
};

baz(ref foo);
foo();
