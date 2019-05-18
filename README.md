# Nestor

Work in progress web server framework for BuckleScript/ReasonML

## Motivation

Currently, if you want to write a bucklescript-based back-end, the most viable
option is to use `express`, or

None of these frameworks follow a functional idioms. E.g., when using express, it
is quite idiomatic to write middlewares that mutate the `Request` object.

The philosophy of this framework is that a middleware doesn't mutate the
context, but passes data on to the next middleware using monadic/kleisli
composition.

## Intended API

This library is not coded yet, but the vision is that you should be able to
create an app in this style:

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
