# Nestor

Work in progress web server framework for BuckleScript/ReasonML that works
directly on top of the native `http` module in node.js.

Idiomatic stronly typed FP code is prioritized over run-time performance.

## Motivation

Currently, if you want to write a bucklescript-based back-end, the most viable
is to use bucklescript bindings to existing node.js frameworks, like `express`.

Unfortunately, express is not geared towards functional programming paradigms, e.g.
it is quite idiomatic to write middlewares that mutate the `Request` object.

The philosophy of this framework is that a middleware doesn't mutate the
context, but passes data on to the next middleware using monadic/kleisli
composition.

## Intended API

This is only work in progress.

The following only represents an _intent_ of the usage of this library.

```reasonml
let findUser = (id, request) => {
  userRepository.find(id)
  |> Async.bind(fun
    | None => Response.render(~status=404, ())
    | Some(user) => Response.next(user)
  )
}

let renderUser = (user, request) => {
  Response.render(~contentType="application/json", encodeUser(user));
}

let apiRouter = router [
  get("/users") >=> renderAllUsers,
  get("/users/:id") >=> decodePathParam("id") >=> findUser >=> renderUser,
]

let application = router [
  path("/api") >=> apiRouter,
  // ensureAuthentication would inspect authentication tokens, etc, and
  // pass the relevant data to the next layer
  path("/private") >=> ensureAuthentication >=> privateRouter
]
```

## Fundamentals

### Middleware

The fundamental type in this library is the 'middleware' poorly named as this
also refer to the parts that generate a response.

The middleware has the type;

```reasonml
type middleware('a, 'b) = 'a => request => response('b)
```

So you can receive some data of type `'a` from the previous middleware - and you
could pass some data of type `'b` to the next middelware.

Due to the asynchronous nature of node.js, the `response` allows for both
asynchronous and synchronous responses.

This structure allows easy composition of parts using kleisli composition, the
result being a middleware too.

### Creating an HTTP server

As the web server is just a composition of middlewares, and a composition of
middlewares is in itself just a middleware, we just need to serve a middleware.

the function `createServer` takes a middleware and returns an
`http.requestListener` function, that works directly with the native `http` module.

``` reasonml
let middleware = path("path") >=> sendText("Hello from /path");
let server = createServer(middleware);

%raw
{|
const http = require('http');
const httpServer = http.createServer(server);
httpServer.listen(4000);
|};
```

I've combined this with raw js code to make it explicit how this integrates to
the native http module.

## Inspiration

This library is slightly inspired by Suave, but with some important changes.
Suave allows the library to store data in a `Context` object. As you retrieve
data, you need to dynamically cast to the expected type, and the .net runtime
would verify type compatibility (throwing an exception if it doesn't )

This solution is not idiomatic in a strongly typed functional programming language.

But it gets even worse with ReasonML/OCaml. The compiler uses the type system
for type checking, but type information is removed from the compiled code.

Because of that, the system cannot verify the type at
runtime, so a type mismatch would lead to some very, very, very nasty bugs down
the line.
