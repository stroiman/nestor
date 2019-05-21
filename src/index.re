module Request = {
  type t = {
    cookies: Js.Dict.t(string),
    path: list(string),
  };

  let getCookie = (name, req) => req.cookies->Js.Dict.get(name);

  let from_native_request = (req: Http.Request.t) => {
    let url = Http.Request.url(req) |> Http.Url.parse;
    let headers = Http.Request.headers(req);
    Js.log2("Headers", headers);
    let cookies =
      headers->Js.Dict.get("cookie")
      |> (
        fun
        | Some(x) => Http.Cookie.parse(x)
        | None => Js.Dict.empty()
      );
    let path =
      Http.Url.path(url)
      |> Js.String.split("/")
      |> Array.to_list
      |> List.filter(x => x != "");

    {path, cookies};
  };
};

module Response = {
  type t = {httpResponse: Http.Response.t};

  let from_native_response = (res: Http.Response.t) => {httpResponse: res};

  let send = (str, t) => t.httpResponse |> Http.Response.end_(str);
};

module Handler = {
  type response('a) =
    | CannotHandle
    | Response(Response.t => unit)
    | Continue('a, Request.t);
  type t('a) = Request.t => response('a);

  let (>>=) = (x: t('a), f: 'a => t('b)): t('b) =>
    req =>
      switch (x(req)) {
      | CannotHandle => CannotHandle
      | Response(f) => Response(f)
      | Continue(data, req) => f(data, req)
      };

  type handlerM('a, 'b) = 'a => t('b);

  let (>=>) = (f: handlerM('a, 'b), g: handlerM('b, 'c)): handlerM('a, 'c) =>
    x => (f(x) >>= g: t('c));
};

open Handler;

let path = p: handlerM('a, 'a) =>
  (data, req) =>
    Request.(
      switch (req.path) {
      | [x, ..._] when x == p => Continue(data, req)
      | _ => CannotHandle
      }
    );

let sendText = text: handlerM('a, unit) =>
  (_, _) => Handler.Response(Response.send(text));

let rec choose = (routes: list(handlerM('a, 'b))): handlerM('a, 'b) =>
  (z: 'a, req) =>
    switch (routes) {
    | [] => CannotHandle
    | [x, ...xs] =>
      switch (x(z, req)) {
      | CannotHandle => choose(xs, z, req)
      | x => x
      }
    };

let tryGetCookie = name: handlerM('a, option(string)) =>
  (_, req) => {
    let cookie = Request.getCookie(name, req);
    Continue(cookie, req);
  };

let getCookie =
    (
      ~onMissing: handlerM(unit, unit),
      ~onFound: handlerM(string, unit),
      name,
    )
    : handlerM(unit, unit) =>
  (x: unit, req) =>
    switch (Request.getCookie(name, req)) {
    | Some(cookie) => onFound(cookie, req)
    | None => onMissing(x, req)
    };

let createServerM = (handleFunc: handlerM('a, 'b)): Http.requestListener =>
  (req, res) => {
    let response = Response.from_native_response(res);
    switch (handleFunc((), Request.from_native_request(req))) {
    | CannotHandle => ()
    | Continue(_) => ()
    | Response(handler) => handler(response)
    };
  };
