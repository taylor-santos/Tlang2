
Functor := class<T> {
    map : func<S>(fn: func(T) => S) => [this]<S>;
};

List := class<T>: Functor<T>{};

IntList := new List<int>();
half := func(i: int) => float {
    return i / 2.;
};
halfList := IntList.map(half);

Pair := class<T, S> {
    first: T;
    second: S;
};

text := "Hello, world!";