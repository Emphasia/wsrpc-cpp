import sys
import time
import asyncio
import aiohttp
import orjson
from loguru import logger


format = "<green>{time:YYYY-MM-DD HH:mm:ss.SSS}</green> | <level>{level: <8}</level> | <level>{message}</level>"
logger.configure(handlers=[dict(sink=sys.stderr, level="INFO", format=format, backtrace=False, diagnose=True)])


class AsyncClient:

    def __init__(self, url: str):
        self.url = url

    async def __aenter__(self):
        self.id = 0
        self.msgs: dict = dict()
        self.newmsg = asyncio.Condition()
        self.session = aiohttp.ClientSession()
        self.client = await self.session.ws_connect(self.url)
        self.handler = asyncio.create_task(self.rsv())
        return self

    async def __aexit__(self, exc_type, exc, tb):
        self.handler.cancel()
        await self.client.close()
        await self.session.close()

    async def call(self):
        assert self.client
        self.id += 1
        id = self.id
        t = time.time()
        await self.client.send_str(f'{{"id":"{str(id)}","method":"{sys.argv[2]}","params":{{}}}}')
        logger.debug(f"sent: {id}")
        async with self.newmsg:
            await self.newmsg.wait_for(lambda: id in self.msgs)
            rsp = self.msgs.pop(id)
        assert self.client and rsp
        tcost = time.time() - t
        assert tcost >= 0
        logger.debug(f"{id:>10} 请求耗时: {tcost*1000:.1f} 毫秒")
        return rsp

    async def rsv(self):
        async for msg in self.client:
            if msg.type != aiohttp.WSMsgType.TEXT:
                continue
            rsp = msg.json(loads=orjson.loads)
            id = int(rsp['id'])
            logger.debug(f"received: {id}")
            async with self.newmsg:
                self.msgs[id] = rsp
                self.newmsg.notify_all()


async def main(target):

    async with AsyncClient(url=target) as client:

        tasks = set()
        t = time.time()
        for i in range(1000):
            tasks.add(client.call())
            await asyncio.sleep(0.001)
            if i % 10 == 0:
                await asyncio.gather(*tasks)
                tasks.clear()
                if i % 100 == 0:
                    logger.info(f"{i} requests done")
        await asyncio.gather(*tasks)
        tcost = time.time() - t
        print(f": {tcost:.3f}")


if __name__ == '__main__':
    target = sys.argv[1]
    asyncio.run(main(target))
    logger.info("done")
