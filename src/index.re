module Request = {
  type t = {
    cookies: Js.Dict.t(string),
    path: list(string),
  };

  let getCookie = (name, req) => req.cookies->Js.Dict.get(name);

  let getPath = _ => "/users/john";

  let from_native_request = req => {
    open NodeModules;
    let url = Http.Request.url(req) |> Url.parse;
    let headers = Http.Request.headers(req);
    let cookies =
      headers->Js.Dict.get("cookie")
      |> (
        fun
        | Some(x) => Cookie.parse(x)
        | None => Js.Dict.empty()
      );
    let path =
      Url.path(url)
      |> Js.String.split("/")
      |> Array.to_list
      |> List.filter(x => x != "");

    {path, cookies};
  };
};

module Response = {
  type t = {
    status: int,
    text: string,
  };

  let empty = {status: 200, text: ""};

  let send = (str, res) => {...res, text: str};
};

module Handler = {
  type response('a) =
    | CannotHandle: response('a)
    | Response(Response.t => Response.t): response('a)
    | Continue('a, Request.t): response('a)
    | Stop('b, 'b => response('a)): response('a)
    | Async((response('a) => unit) => unit): response('a);

  type t('a) = Request.t => response('a);

  let rec (>>=) = (x: t('a), f: 'a => t('b)): t('b) =>
    req =>
      switch (x(req)) {
      | CannotHandle => CannotHandle
      | Response(f) => Response(f)
      | Continue(data, req) => f(data, req)
      | Stop(x, f) => f(x)
      | Async(asyncResult) =>
        Async(
          cb =>
            asyncResult((result: response('a)) =>
              cb(req |> ((_ => result) >>= f))
            ),
        )
      };

  type handlerM('a, 'b) = 'a => t('b);

  let (>=>) = (f, g, x) => f(x) >>= g;
};

open Handler;

let path = (p): handlerM('a, 'a) =>
  (data, req) =>
    Request.(
      switch (req.path) {
      | [x, ...xs] when x == p => Continue(data, {...req, path: xs})
      | _ => CannotHandle
      }
    );

let sendText = (text): handlerM('a, 'b) =>
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

let tryGetCookie = (name): handlerM('a, option(string)) =>
  (_, req) => {
    let cookie = Request.getCookie(name, req);
    Continue(cookie, req);
  };

let getCookie =
    (~onMissing: handlerM('a, string), name): handlerM('a, string) =>
  (x: 'a, req) =>
    switch (Request.getCookie(name, req)) {
    | Some(cookie) => Continue(cookie, req)
    | None => Stop(x, fn => onMissing(fn, req))
    };

let createServer = handleFunc =>
  (. req, res) => {
    let rec handleResponse = response =>
      switch (response) {
      | CannotHandle =>
        /* If the handler cannot handle the request, then doing nothing might
           be the most sensible thing to do, as other request handlers could have
           been attached to the http server. */
        ()
      | Continue(_) => ()
      | Response(handler) =>
        let r = handler(Response.empty);
        res |> NodeModules.Http.Response.end_(r.text);
      | Stop(x, f) => handleResponse(f(x))
      | Async(cb) => cb(handleResponse)
      };
    handleResponse(handleFunc((), Request.from_native_request(req)));
  };
