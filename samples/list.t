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
};

a, b, t := *(1, 2, (4, 5, 6));
*/
a, b: int;
b := 5;
Foo := class {
    x: int;
};
a := b;
