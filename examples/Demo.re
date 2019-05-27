open Index;
open Index.Handler;

let delay = (x, req, _res, cb) =>
  Js.Global.setTimeout(() => cb(Continue(x, req)), 1000) |> ignore;

let server =
  router([
    path("path") >=> (() => sendText("Hello from /path")),
    path("delay")
    >=> delay
    >=> (() => sendText("This renders after 1 second")),
    () => sendText("Not found"),
  ])
  |> createServer;

%raw
{|
const http = require('http');
let httpServer = http.createServer(server);
httpServer.listen(4000);
|};
