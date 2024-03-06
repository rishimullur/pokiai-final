const fs = require("file-system");
const http = require("http");
const path = require("path");
const { parse } = require("querystring");
const axios = require("axios"); // You will need axios for making POST requests, ensure to install it

const recordingsFolder = "/Users/rishi/dev/XIAO-ESP32S3Sense-Speech2ChatGPT/NodejsServer/resources";
const transcriptionsFile = path.join(recordingsFolder, "transcriptions.txt");

// Import necessary modules for speech to text
const speech = require('@google-cloud/speech');
const speechClient = new speech.SpeechClient();
let lastReadTime = 0;

const server = http.createServer();

server.on("request", (request, response) => {
    if (request.method === "GET" && request.url === "/transcription") {
        const stats = fs.statSync(transcriptionsFile);
        const fileModifiedTime = new Date(stats.mtime).getTime();
    
        if (fileModifiedTime > lastReadTime) {
            // There are new transcriptions since the last check
            const transcriptions = fs.readFileSync(transcriptionsFile, { encoding: 'utf8' });
            lastReadTime = fileModifiedTime; // Update lastReadTime to the current file modification time
    
            response.writeHead(200, { "Content-Type": "text/plain" });
            response.end(transcriptions);
        } else {
            // No new transcriptions
            response.writeHead(204); // No Content
            response.end();
        }
    
    } else if (request.method == "POST" && request.url === "/uploadAudio") {
        let data = [];
        request.on('data', chunk => {
            data.push(chunk);
        }).on('end', async () => {
            // Concatenate the data chunks into a Buffer
            const buffer = Buffer.concat(data);

            // Generate a unique filename for the uploaded file
            const timestamp = Date.now();
            const filename = path.join(recordingsFolder, `${timestamp}.wav`);

            // Save the buffer to a file
            fs.writeFileSync(filename, buffer);

            try {
                // Transcribe the audio file
                const transcription = await speechToTextAPI(filename);

                // Append the transcription to the transcriptions file
                fs.appendFileSync(transcriptionsFile, transcription + "\n");

            //     // Making a POST call to the ESP32 with the transcribed text
            //     await axios.post('http://10.19.185.226:80', { text: transcription });

                response.writeHead(200, { "Content-Type": "text/plain" });
                response.end("Audio processed and transcribed successfully.");
            } catch (error) {
                console.error("Error processing file:", error);
                response.writeHead(500, { "Content-Type": "text/plain" });
                response.end("Internal Server Error");
            }
        });
    } else {
        response.writeHead(405, { "Content-Type": "text/plain" });
        response.end("Method Not Allowed");
    }
});

async function speechToTextAPI(fileName) {
    // Reads a local audio file and converts it to base64
    const file = fs.readFileSync(fileName);
    const audioBytes = file.toString("base64");

    const request = {
        audio: { content: audioBytes },
        config: {
            encoding: "LINEAR16",
            sampleRateHertz: 16000,
            languageCode: "en-US"
        },
    };

    // Detects speech in the audio file
    const [response] = await speechClient.recognize(request);
    const transcription = response.results.map(result => result.alternatives[0].transcript).join("\n");
    console.log(`Transcription: ${transcription}`);
    return transcription;
}

const port = 8888;
server.listen(port);
console.log(`Server listening on port ${port}`);
