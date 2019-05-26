module Request = {
  type t = {
    cookies: Js.Dict.t(string),
    path: string,
  };

  let getCookie = (name, req) => req.cookies->Js.Dict.get(name);

  let getPath = req => req.path;

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
    let path = Url.path(url);

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
type httpResultx('a) =
  | CannotHandle
  | Done(Response.t => Response.t)
  | Continue('a, Request.t);

type httpResult('a) = (httpResultx('a) => unit) => unit;

module Handler = {
  type t('a) = (Request.t, Response.t) => httpResult('a);
  type m('a, 'b) = 'a => t('b);

  let rec (>>=) = (x: t('a), f: 'a => t('b)): t('b) =>
    (req, res) => {
      let asyncResult = x(req, res);
      cb =>
        asyncResult(
          fun
          | Continue(x, req) => f(x, req, res, cb)
          | x => cb(x),
        );
    };

  let (>=>) = (f: m('a, 'b), g: m('b, 'c), x: 'a): t('c) => f(x) >>= g;
};

type middleware('a, 'b) = 'a => Handler.t('b);

let path = searchPath: middleware('a, 'a) =>
  (data, req, res, cb) => {
    open Request;
    open Js;
    let requestPath = req |> getPath;
    let length = String.length(searchPath);
    let newPath = String.substr(~from=length, requestPath);
    String.startsWith(searchPath, requestPath)
    && String.startsWith("/", requestPath) ?
      cb(Continue(data, {...req, path: newPath})) : cb(CannotHandle);
  };

let sendText = (text, _, _, cb) => cb(Done(Response.send(text)));

let rec router = (routes, data, req, res, cb) =>
  switch (routes) {
  | [] => cb(CannotHandle)
  | [route, ...routes] =>
    route(
      data,
      req,
      res,
      fun
      | CannotHandle => router(routes, data, req, res, cb)
      | x => cb(x),
    )
  };

let tryGetCookie = name: middleware('a, option(string)) =>
  (_, req, res, cb) => {
    let cookie = Request.getCookie(name, req);
    cb(Continue(cookie, req));
  };

let getCookie =
    (~onMissing: middleware('a, string), name): middleware('a, string) =>
  (x: 'a, req, res) =>
    switch (Request.getCookie(name, req)) {
    | Some(cookie) => (cb => cb(Continue(cookie, req)))
    | None => onMissing(x, req, res)
    };

let createServer = (handleFunc: middleware('a, 'b)) =>
  (. req, res) => {
    let req = Request.from_native_request(req);
    let result = handleFunc((), req, Response.empty);
    result(
      fun
      | CannotHandle =>
        /* If the handler cannot handle the request, then doing nothing might
           be the most sensible thing to do, as other request handlers could have
           been attached to the http server. */
        ()
      | Continue(_) =>
        /* Should probably result in a HTTP 500 status - the server code is misconfigured */
        ()
      | Done(handler) => {
          let r = handler(Response.empty);
          res |> NodeModules.Http.Response.end_(r.text);
        },
    );
  };
