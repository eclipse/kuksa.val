import asyncio,json,time
from threading import Thread
import websockets


class vssclient:
    def __init__(self, uri, token):
        self.uri=uri
        self.token=token
        self.request=10
        
        loop = asyncio.new_event_loop()
        t = Thread(target=self.start_background_loop, args=(loop,), daemon=True)
        t.start()


        
    def start_background_loop(self,loop):
        asyncio.set_event_loop(loop)
        self.queue = asyncio.Queue(1000)
        loop.create_task(self.connect(self.uri, loop))
        loop.run_forever()

    async def connect(self, uri,loop):
        print("Connect")
        self.ws=await websockets.connect(uri)
        loop.create_task(self.handleResponse(self.ws))
        loop.create_task(self.sendLoop(self.ws,self.queue))
        loop.create_task(self.authorize())
        print("Connect Done")

    async def authorize(self):
        print("Authorize")
        auth={}
        auth['requestId']=1
        auth['action']="authorize"
        auth['tokens']=self.token
        
        print("send")
        await self.queue.put(json.dumps(auth))
        print("Authorize done")

    
    async def sendLoop(self,ws,queue):
        while True:
            try:
                json = queue.get_nowait() #just calling await get will block for 20 sec and then do 20 at once :-/
            except asyncio.QueueEmpty:
                #print("Empty")
                await asyncio.sleep(0.2)
                continue
            #print("WS send")
            await self.ws.send(json)
            queue.task_done()
           

 

    async def handleResponse(self,ws):
        print("Handle response for {}".format(ws))
        async for message in ws:
            data = json.loads(message)
            if not "action" in data or data['action'] != "set": 
                print("Received {}".format(message))
        print("HandleResponse terminated")

    def push(self,path,value):
        req={}
        req['requestId']=self.request
        self.request+=1
        req['action']="set"
        req['path']=path
        req['value']=str(value)
        self.queue.put_nowait(json.dumps(req))
        
        
