/**
  * Mix multiple http handlers!
  *
  * This example shows that you can mix Nestor with other HTTP frameworks. Here
  * one path is handled by Nestor, another handled by Express.
  *
  * I have NO IDEA if this leaks resources - because this explicitly sets up a
  * request middleware that ignores the request (otherwiser Express will try to
  * respond with a 404 page on all the routes handled by Nestor).
  */
open Index;
open Index.Handler;

let middleware =
  path("nestor")
  >=> router([
        path("a") >=> sendText("Hello from /nestor/a"),
        path("b") >=> sendText("Hello from /nestor/b"),
        sendText("Hello from /nestor"),
      ]);

let reasonServer = createServer(middleware);

%raw
{|
const http = require('http');
const express = require('express');

const app = express();
app.disable("x-powered-by"); // Otherwise, express tries to add the header to ALL requests
app.use("/express", (req, res) => res.send("Hello from Express"));
app.use("/nestor", (req, res) => { /* Ignore nestor routes */ });

const server = new http.Server();
server.on('request', reasonServer);
server.on('request', app);
server.listen(4000);
|};
