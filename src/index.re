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

type httpResult('a) =
  | Async((httpResultx('a) => unit) => unit);

module Handler = {
  type t('a) = Request.t => httpResult('a);
  type m('a, 'b) = 'a => t('b);

  let rec (>>=) = (x: t('a), f: 'a => t('b)): t('b) =>
    req =>
      switch (x(req)) {
      | Async(asyncResult) =>
        Async(
          (
            cb =>
              asyncResult(
                fun
                | Continue(x, req) =>
                  switch (f(x, req)) {
                  | Async(x) => x(cb)
                  }
                | x => cb(x),
                /* result => cb(req |> ((_ => result) >>= f)) */
              )
          ),
        )
      };

  let (>=>) = (f: m('a, 'b), g: m('b, 'c), x: 'a): t('c) => f(x) >>= g;
};

type middleware('a, 'b) = 'a => Handler.t('b);

let path = searchPath: middleware('a, 'a) =>
  (data, req) => {
    open Request;
    open Js;
    let requestPath = req |> getPath;
    let length = String.length(searchPath);
    let newPath = String.substr(~from=length, requestPath);
    String.startsWith(searchPath, requestPath)
    && String.startsWith("/", requestPath) ?
      Async(cb => cb(Continue(data, {...req, path: newPath}))) :
      Async(cb => cb(CannotHandle));
  };

let sendText = (text, _) => Async(cb => cb(Done(Response.send(text))));

let rec router = (routes, data, req) =>
  switch (routes) {
  | [] => Async((cb => cb(CannotHandle)))
  | [x, ...xs] =>
    switch (x(data, req)) {
    | Async(cb) =>
      Async(
        (
          cb2 =>
            cb(
              fun
              | CannotHandle =>
                switch (router(xs, data, req)) {
                | Async(x) => x(cb2)
                }
              | x => cb2(x),
            )
        ),
      )
    }
  };

let tryGetCookie = name: middleware('a, option(string)) =>
  (_, req) => {
    let cookie = Request.getCookie(name, req);
    Async(cb => cb(Continue(cookie, req)));
  };

let getCookie =
    (~onMissing: middleware('a, string), name): middleware('a, string) =>
  (x: 'a, req) =>
    switch (Request.getCookie(name, req)) {
    | Some(cookie) => Async((cb => cb(Continue(cookie, req))))
    | None => onMissing(x, req)
    };

let createServer = (handleFunc: middleware('a, 'b)) =>
  (. req, res) => {
    let rec handleSyncResponse = response =>
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
      };
    let rec handleAsyncResponse = response =>
      switch (response) {
      | Async(x) => x(handleSyncResponse)
      };
    handleAsyncResponse(handleFunc((), Request.from_native_request(req)));
  };
