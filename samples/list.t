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

a, b, c := 1, 2, 3;

text := "Hello, world!";

foo();

t: (int, int, string);
