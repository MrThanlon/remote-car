#!/usr/bin/env node

"use strict"

const conf = require("./config")
const net = require('net')
const Koa = require('koa')
const koaStatic = require('koa-static')
const koaMount = require('koa-mount')
const websockify = require('koa-websocket')
const child = require('child_process')
const fs = require('fs')

// Prevent exit
process.on('uncaughtException', function (err) {
    console.error(err.stack);
    console.log("Node NOT Exiting...");
    setTimeout(() => backend.kill(), 1000)
});

// TCP listen
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
    socket.on("error", function (e) {
        console.debug(`[TCP] Socket Error: `, e)
    })

    socket.on("data", function (data) {
        // console.debug(`[TCP] received: `, data)
        // 转发
        wsPool = wsPool.filter(v => {
            try {
                v.websocket.send(data)
                return true
            } catch (e) {
                return false
            }
        })
        // console.debug(`[TCP] transfered`)
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
const staticFile = new koaStatic(__dirname + '/remote-car-fe/dist')
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
    ctx.websocket.on("message", message => {
        console.debug(`[WebSocket] received: `, message)
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
let lastPosUpdate = Date.now()
if (fs.existsSync(__dirname + '/backend/backend')) {
    //文件存在，运行
    let backend = child.spawn(__dirname + '/backend/backend')
    console.debug(`[GPIO] Running...`)
    backend.stdout.on('data', (data) => {
        const s = data.toString()
        console.debug(`[GPIO] receive:`, s)
        // 解析
        const pos = /^(\-?\d+\.?\d+)\s(\-?\d+\.?\d+)/.exec(s)
        if (pos) {
            lastPosUpdate = Date.now()
            const x = Math.round(pos[1] * 100)
            const y = Math.round(pos[2] * 100)
            const id = randomID()
            let b = [0x99, 0xbc, x >> 8, x & 0xff, y >> 8, y & 0xff, id >> 8, id & 0xff]
            b.push(b.reduce((pre, cur) => pre + cur, 0) & 0xff)
            // 发送
            wsPool = wsPool.filter(v => {
                try {
                    v.websocket.send(Buffer.from(b))
                    return true
                } catch (e) {
                    return false
                }
            })
        }
    })
    backend.on('close', () => {
        // 重新拉起
        lastPosUpdate = Date.now()
        console.debug(`[GPIO] exited, reopening...`)
        backend = child.spawn('./backend/backend')
        console.debug(`[GPIO] reopened`)
        backend.stdout.on('data', (data) => {
            const s = data.toString()
            console.debug(`[GPIO] receive:`, s)
            // 解析
            const pos = /^(\-?\d+\.?\d+)\s(\-?\d+\.?\d+)/.exec(s)
            if (pos) {
                const x = Math.round(pos[1] * 300 + 150)
                const y = Math.round(pos[2] * 300 + 150)
                const id = randomID()
                let b = [0x99, 0xbc, x >> 8, x & 0xff, y >> 8, y & 0xff, id >> 8, id & 0xff]
                b.push(b.reduce((pre, cur) => pre + cur, 0) & 0xff)
                // 发送
                wsPool = wsPool.filter(v => {
                    try {
                        v.websocket.send(Buffer.from(b))
                        return true
                    } catch (e) {
                        return false
                    }
                })
            }
        })
    })
    // 当500毫秒内没收到数据的时候转一下/重新拉起

    setInterval(() => {
        if (socketPoll.length > 0 && Date.now() - lastPosUpdate > 2000) {

            console.debug(`[GPIO] send rotate`)
            socketPoll = socketPoll.filter(v => {
                try {
                    v.write(Buffer.from([0x99, 0x34, 0x01, 0x2c, 0x00, 0x00, 0x00, 0x00, 0xfa]))
                    return true
                }
                catch (e) {
                    return false
                }
            })
            setTimeout(() => {
                socketPoll = socketPoll.filter(v => {
                    try {
                        v.write(Buffer.from([0x99, 0x34, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 205]))
                        return true
                    }
                    catch (e) {
                        return false
                    }
                })
            }, 1000)
            lastPosUpdate = Date.now()

            //console.debug(`[GPIO] exited, reopening...`)
            //backend.kill()
        }
    }, 2000)
}