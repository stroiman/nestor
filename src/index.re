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

/** Reresents the result of a middleware action */
type httpResult('a) =
  | CannotHandle
  | Done(Response.t => Response.t)
  | Continue('a, Request.t)
  | Stop(httpResult('a))
  | Async((httpResult('a) => unit) => unit);

module Handler = {
  type t('a) = Request.t => httpResult('a);

  let rec (>>=) = (x: t('a), f: 'a => t('b)): t('b) =>
    req =>
      switch (x(req)) {
      | CannotHandle => CannotHandle
      | Done(f) => Done(f)
      | Continue(data, req) => f(data, req)
      | Stop(x) => x
      | Async(asyncResult) =>
        Async(
          (cb => asyncResult(result => cb(req |> ((_ => result) >>= f)))),
        )
      };

  let (>=>) = (f, g, x) => f(x) >>= g;
};

type middleware('a, 'b) = 'a => Handler.t('b);

let path = p: middleware('a, 'a) =>
  (data, req) =>
    Request.(
      switch (req.path) {
      | [x, ...xs] when x == p => Continue(data, {...req, path: xs})
      | _ => CannotHandle
      }
    );

let sendText = text: middleware('a, 'b) =>
  (_, _) => Done(Response.send(text));

let rec choose = (routes, data, req) =>
  switch (routes) {
  | [] => CannotHandle
  | [x, ...xs] =>
    switch (x(data, req)) {
    | CannotHandle => choose(xs, data, req)
    | x => x
    }
  };

let tryGetCookie = name: middleware('a, option(string)) =>
  (_, req) => {
    let cookie = Request.getCookie(name, req);
    Continue(cookie, req);
  };

let getCookie =
    (~onMissing: middleware('a, string), name): middleware('a, string) =>
  (x: 'a, req) =>
    switch (Request.getCookie(name, req)) {
    | Some(cookie) => Continue(cookie, req)
    | None => Stop(onMissing(x, req))
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
      | Continue(_) =>
        /* Should probably result in a HTTP 500 status - the server code is misconfigured */
        ()
      | Done(handler) =>
        let r = handler(Response.empty);
        res |> NodeModules.Http.Response.end_(r.text);
      | Stop(x) => handleResponse(x)
      | Async(cb) => cb(handleResponse)
      };
    handleResponse(handleFunc((), Request.from_native_request(req)));
  };
