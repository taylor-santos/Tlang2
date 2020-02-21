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
