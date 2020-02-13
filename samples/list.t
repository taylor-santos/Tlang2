/*
Functor := class<T> {
    map : func<S>(fn: func(T) => S) => [this]<S>;
};

List := class<T>: Functor<T>{};

IntList := new List<int>();
half := func(i: int) => float {
    return i / 2.;
};
halfList := IntList.map(half);

comp := func<A, B, C>(
    f2: func<B, C>(B) => C,
    f1: func<A, B>(A) => B
) => func(A) => C {
  return func(a: A) => C {
    return f2(f1(a));
  };
};

Pair := class<T, S> {
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
    Foo := func() {
        x := 0;
        y := "Foo";
    };
}

a, b, t := *(1, 2, (4, 5, 6));
*/

/*
list: []int;

A := class {
    a: int;
};

B := class {
    a: func() => int;
    b: func();
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

Foo := class {
    a: int;
    x: int;
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

main := func(b: bool) => int {
    if (b) {
        a := new Foo();
        fn(a);
    } else {
        a := new Bar();
        fn(a);
    }
    fn(a);
    return 0;
};
