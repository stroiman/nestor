# Nestor

Work in progress web server framework for BuckleScript/ReasonML that works
directly on top of the native `http` module in node.js.

Idiomatic strongly typed FP code is prioritized over run-time performance.

This project is in an exploratory state, the goal right now is to find a _great_
API. So a lot of things might change

## Motivation

Currently, if you want to write a bucklescript-based back-end, the most viable
option is to use bucklescript bindings to existing node.js frameworks, like `express`.

Unfortunately, express is not geared towards functional programming paradigms,
e.g. it is a common practice to mutate the express `Request` object.

The philosophy of this framework is that a middleware doesn't mutate the
context, but passes data on to the next middleware using monadic composition.

## Intended API

This library is work in progress, and the following only represents an _intent_ of the usage of this library.

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

let updateUser = (id, userJson, request) => {
  userRepository.update(id, userJson)
  |> Async.bind(fun
    | Ok(user) => Response.render( /* ... */ )
    | Error(UserNotFound) => Response.render(~status=404)
    | _ => Response.render(~status=500)
  )
}

let getBodyJson = (request) => {
  // Retrieve the body stream and return as a Js.Json.t
}

let apiRouter = router [
  get("/users") >=> renderAllUsers,
  get("/users/:id") >=> decodePathParam("id") >=> findUser >=> renderUser,
  post("/users/:id") >=> decodePathParam("id") >=> userId => getBodyJson >=> decodeUser >=> updateUser(userId)
]

// Alternatively

let apiRouter = router [
  path("/users") >> router [
    get("/") >> renderAllUsers,
    path(":/id") >> decodePathParam("id") >>= userId => router [
      mehod("GET") >> findUser(userId) >>= renderUser,
      method("POST") >> getBodyJson >>= decodeUser >>= updateUser(userId)
    ]
  ]
]

let application = router [
  path("/api") >> apiRouter,
  // ensureAuthentication would inspect authentication tokens, etc, and
  // pass the relevant data to the next layer
  path("/private") >> ensureAuthentication >>= privateRouter
]
```

## Fundamentals

### Handler

Requests are handled by a `Handler.t` which has the signature:

```
type t('a) = (Request.t, Response.t) => Async(HandlerResult('a))
```

So the `HandlerResult` _can_ carry information. This could be looking up the
value of a cookie.

### Async

The type `async` represents a result that will be evaluated asynchronously. The
actual type is a function that accepts TWO callbacks, one for successful result,
and one to handle if an exception was thrown.

```
type async('a) = (('a => unit, exn => unit)) => unit
```

Exceptions here are only intended for _truly exceptional_ conditions, e.g. a
broken database connection.

### Result type

```reason
type httpResult('a) =
  | CannotHandle
  | Done(Response.t)
  | Continue('a)
  | NewContext('a, Request.t, Response.t);
```

* `CannotHandle` This handler is unable to handle the request. E.g. a handler of
    http `GET` requests would return this when called with an HTTP `POST`
    request.
* `Done` A response has been generated
* `Continue` The handler was successful but have not generated a response. The
    next handler in the chain should be called. This can carry a payload, e.g.
    the value of a cookie.
* `NewContext` Like `Continue` but with some changes to the `Request` (we may
    have filtered on a subpath) or `Response` (We may have added a
    cookie/response header)

### Composing handlers

#### Operator `>>`

Handlers can be directly composed using the `>>` operator. This is most usable
when dealing with handlers that that don't carry values in the return type, e.g.
request filtering:

```
path("/foo") >> method("GET") >> router([ ... ])
```

#### Operator `>>=`

This is the monadic bind operator, which is usable when a handler carries a
value:

```
tryGetCookie("username")
>>= cookie => switch(cookie) {
  | Some(userName) => sendText("Hello, " ++ userName);
  | None => sendText("Hello, mysterios one");
})
```

#### Operator `>=>`

This is the kleisli composition of handler-generating functions:

```reason
let ( >=> ): ('a => t('b), 'b => t('c)) => ('a => t('c))

let getUser: (string => Handler.t(user));

let renderUser: (user => Handler.t(unit));

let getAndRenderUser: (string => Handler.t(unit)) = getUser >=> renderUser;
```

**Note:** This was how I originally believed handlers should be composed, but
working with the code have pushed this operator to be far less important.

### Creating an HTTP server

As the web server is just a composition of handlers, and a composition of
handlers is in itself just a handler, we just need to serve a handler.

The function `createServer` takes a middleware and returns an
`http.requestListener` function, that works directly with the native `http` module.

``` reasonml
let middleware = path("path") >=> sendText("Hello from /path");
let server = createServer(middleware);

%raw
{|
// Raw JS code to explicitly show how this interacts with node's HTTP module.
const http = require('http');
const httpServer = http.createServer(server);
httpServer.listen(4000);
|};
```

## Inspiration

This library is slightly inspired by Suave, but with some important changes.
Suave allows the library to store data in a `Context` object. As you retrieve
data, you need to dynamically cast to the expected type, and the .net runtime
would verify type compatibility (throwing an exception if it doesn't )

This solution is not idiomatic in a strongly typed functional programming
language, where you would like the type checker to catch the bug at compile time
instead.

But it gets even worse with ReasonML/OCaml. The compiler uses the type system
for type checking, but does not add run-time type information to the program.

Because of that, the system cannot verify the type at runtime, so a type
mismatch would lead to some very, very, very nasty bugs down the line.
