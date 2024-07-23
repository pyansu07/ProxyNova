
# ProxyNova

Proxynova is a high-performance proxy server developed in C using Socket Programming and Computer Networking concepts. The server handles client requests sequentially and caches responses  to improve network efficiency.

## Features :

- Manages client requests `sequentially` to ensure efficient processing.
- `LRU Caching Mechanism`: Utilizes `linked lists` and structs to implement an LRU cache, achieving `O(1)` time complexity for data access and updates.
- `Network Efficiency`: If a website's content is cached, far less data needs to be transferred in and out of the website's host server, resulting in lower bandwidth costs.Hence, `Reduces bandwidth` usage and `improves response times` for cached resources.


## Note :
- Large websites may not be stored in the cache due to fixed-size cache elements.
- Cannot manage multiple clients at the same time.
- Only supports `HTTP/1` and `HTTP/1.1` protocols.
# Installation

## Step-1: Clone the repository:

```bash
git clone <---link--->
cd proxynova

```
## Step-2: Compile the server:
```bash
make all

```

## Step-3:Run the server:

```bash
./proxy <port_number>

#example - ./proxy 8080
```
Open: `http://localhost:8080/https://servertest.online/http`


## How It Works

- Website I am using: `https://servertest.online/http`
![Screenshot (139)](https://github.com/user-attachments/assets/b405bcbf-ca1b-4fa8-9454-b180a7d47a89)


### Initial Request (Cache Miss):
- When a website is accessed for the first time, the proxy server will not find the URL in the cache, resulting in `URL not found`.

![Screenshot (138)](https://github.com/user-attachments/assets/f6d87bc4-ffbb-4d81-9b1b-522944cd31e3)


### Subsequent Requests (Cache Hit):
- When the same website is accessed again, the proxy server will find the `URL in the cache`.
![Screenshot (141)](https://github.com/user-attachments/assets/3f3107f4-17a1-4888-a470-4a383d18c7fc)


### Network Tab Showing Decrease in Response Time

- When a website is accessed for the first time, `Time taken is 21.86s`. 
![Screenshot (144)](https://github.com/user-attachments/assets/ea408d6f-186b-4b02-ab9a-4f1ceedc52d1)


- When the same website is accessed again, `Time taken is 310 ms`. The response time dropped by approximately `98.58%`.
![Screenshot (146)](https://github.com/user-attachments/assets/bb59cdd1-5c1b-4178-8ab1-91058041de9f)


#### Implementation Details

- Implemented in C, this project draws inspiration from  [proxy-server](https://github.com/vaibhavnaagar/proxy-server).
