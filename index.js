#!/usr/bin/node

"use strict"

const conf = require("./config")
const net = require('net')
const Koa = require('koa')
const koaStatic = require('koa-static')
const koaMount = require('koa-mount')
const websockify = require('koa-websocket')

// 防止退出
process.on('uncaughtException', function (err) {
    console.error(err.stack);
    console.log("Node NOT Exiting...");
});

// tcp监听
const tcpServer = new net.createServer()

tcpServer.on("listening", function () {
    console.log(`[TCP] Server opened, port: ${conf.tcpPort}`);
})

// tcp连接池
let socketPoll = []

// 转发
tcpServer.on("connection", function (socket) {
    console.log(`[TCP] Connected from [${socket.remoteAddress}:${socket.remotePort}]`)
    socketPoll.push(socket)

    socket.on("data", function (data) {
        console.debug(`[TCP] received: ${data}`)
        // 转发
        wsPool = wsPool.filter(v => {
            try {
                v.websocket.send(data)
                return true
            } catch (e) {
                return false
            }
        })
        console.debug(`[TCP] transfered`)
    })
})

tcpServer.on("close", function () {
    console.log(`[TCP] Server closed`);
});
tcpServer.on("error", function (err) {
    console.log(`[TCP] Error:${err}`);
})

tcpServer.listen(conf.tcpPort)

function randomID() {
    return Math.round(Math.random() * 0xffff)
}

setInterval(() => {
    const id = randomID()
    socketPoll = socketPoll.filter(v => {
        try {
            v.write(Buffer.from([0x99, 0x78, id >> 8, id & 0xff, (0x99 + 0x78 + (id >> 8) + id & 0xff) & 0xff]))
            return true
        }
        catch (e) {
            return false
        }
    })
    console.debug(`[TCP] send heartbeat, ${socketPoll.length} client(s)`)
}, 10000)

// http+ws监听
// 静态资源
const staticFile = new koaStatic('./remote-car-fe/dist')
const staticServer = new Koa()
staticServer.use(staticFile)
const http = new Koa()
http.use(koaMount('/static', staticServer))
// ws
let wsPool = []
const ws = websockify(http)
ws.ws.use((ctx, next) => {
    //ctx.websocket.send("Connected!")
    console.debug(`[WebSocket] connected from ${ctx.ip}`)
    ctx.websocket.on("message", (message) => {
        console.debug(`[WebSocket] received: ${message}`)
        // echo
        //ctx.websocket.send(message)
        // transfer
        socketPoll = socketPoll.filter(v => {
            try {
                v.write(message)
                return true
            }
            catch (e) {
                return false
            }
        })
        console.debug(`[WebSocket] transfered`)
    })
    wsPool.push(ctx)
    return next(ctx)
})

ws.listen(conf.httpPort)
console.debug(`[HTTP] Server opened, port: ${conf.httpPort}`)

// 子进程监听
