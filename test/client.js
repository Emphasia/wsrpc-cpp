import WebsocketSession from './session.js';

document.addEventListener('DOMContentLoaded', function () {
    const outputDiv = document.getElementById('output');

    function logMessage(message, level = 'INFO') {
        const formattedTime = new Date().toISOString();
        const colors = {
            DEBUG: '#3399ff',
            INFO: '#00cc66'
        };

        const color = colors[level] || '#ffffff';
        outputDiv.innerHTML += `<div style="color:${color}">${formattedTime} | ${level.padEnd(8)} | ${message}</div>`;
    }

    async function runTest() {
        const target = document.getElementById('serverUrl').value || `ws://${window.location.hostname}:9001`;

        try {
            const session = new WebsocketSession(target);

            await session.open();

            const t = Date.now();
            const totalRequests = 1000;
            const batchSize = 30;
            const batchDelay = 10; // milliseconds

            for (let i = 0; i < totalRequests; i += batchSize) {
                const batchPromises = [];
                for (let j = 0; j < batchSize; j++) {
                    if (i + j >= totalRequests) break; // Don't exceed total
                    batchPromises.push(session.call("echo"));
                }
                for (let j = 0; j < batchPromises.length; j++) {
                    await batchPromises[j];
                }
                logMessage(`${i + batchSize} requests done`, 'INFO');
            }

            const tcost = Date.now() - t;
            logMessage(`Total time: ${tcost.toFixed(0)} milliseconds`);

            session.close();

        } catch (error) {
            logMessage(`Error: ${error.message}`, 'ERROR');
        }
    }

    window.runTest = runTest;
});
