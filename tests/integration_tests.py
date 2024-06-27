import subprocess
import requests
import unittest
import time
import sys
import signal

class IntegrationTests(unittest.TestCase):
    server_binary = "./bin/server"
    server_config = "../configs/integration.conf"
    server_port = 80
    server_url = "http://localhost:80"

    @classmethod
    def setUpClass(cls):
        cls.server_process = subprocess.Popen([cls.server_binary, cls.server_config])
        time.sleep(1)  

    @classmethod
    def tearDownClass(cls):
        cls.server_process.terminate()
        cls.server_process.wait()

    def test_echo(self):
        session = requests.Session()
        response = session.get(self.server_url + "/echo", timeout=5)
        print(response.text)
        expected_response = "GET /echo HTTP/1.1"

        self.assertEqual(response.status_code, 200)
        self.assertEqual(response.headers['Content-Type'], 'text/plain')
        self.assertIn(expected_response, response.text)

    def test_static(self):
        session = requests.Session()
        response = session.get(self.server_url + "/static/Test0.html", timeout=5)
        print(response.text)
        expected_response = "<!DOCTYPE html>\n<html>\n    <head>\n        <title>TEST</title>\n    </head>\n    <body>\n        STUFF\n    </body>\n</html>\n"

        self.assertEqual(response.status_code, 200)
        self.assertEqual(response.headers['Content-Type'], 'text/html')
        self.assertIn(expected_response, response.text)

    def test_static_markdown(self):
        session = requests.Session()

        parsed = "<h1>TEST</h1>\n<p>HERE IS <em>SOME</em> STUFF AND <strong>MORE</strong> STUFF AND <em><strong>EVEN MORE</strong></em> STUFF</p>\n<h2>LISTS</h2>\n<h3>ORDERED LIST</h3>\n<ol>\n<li>THIS</li>\n<li>IS</li>\n<li>A</li>\n<li>LIST</li>\n</ol>\n<h3>UNORDERED LIST</h3>\n<ul>\n<li>THIS</li>\n<li>IS</li>\n<li>AN</li>\n<li>UNORDERED</li>\n<li>LIST</li>\n</ul>\n<h2>LINK</h2>\n<p>THIS IS A VERY <a href=\"https://www.youtube.com/watch?v=dQw4w9WgXcQ\">IMPORTANT LINK</a></p>\n<h2>CODE</h2>\n<pre><code># THIS IS A BLOCK OF CODE\nsudo rm -rf /\n</code></pre>\n";
        raw = "HERE IS *SOME* STUFF AND **MORE** STUFF AND ***EVEN MORE*** STUFF\r\n\r\n## LISTS\r\n\r\n### ORDERED LIST\r\n\r\n1. THIS\r\n2. IS\r\n3. A\r\n4. LIST\r\n\r\n### UNORDERED LIST\r\n\r\n- THIS\r\n- IS\r\n- AN\r\n- UNORDERED\r\n- LIST\r\n\r\n## LINK\r\n\r\nTHIS IS A VERY [IMPORTANT LINK](https://www.youtube.com/watch?v=dQw4w9WgXcQ)\r\n\r\n## CODE\r\n\r\n```\r\n# THIS IS A BLOCK OF CODE\r\nsudo rm -rf /\r\n```\r\n";
        bad_request = "Bad request";

        response = session.get(self.server_url + "/static/Test5.md", timeout=5)
        print(response.text)
        self.assertEqual(response.status_code, 200)
        self.assertEqual(response.headers['Content-Type'], 'text/html')
        self.assertIn(parsed, response.text)

        response = session.get(self.server_url + "/static/Test5.md?raw=false", timeout=5)
        print(response.text)
        self.assertEqual(response.status_code, 200)
        self.assertEqual(response.headers['Content-Type'], 'text/html')
        self.assertIn(parsed, response.text)

        response = session.get(self.server_url + "/static/Test5.md?raw=true", timeout=5)
        print(response.text)
        self.assertEqual(response.status_code, 200)
        self.assertEqual(response.headers['Content-Type'], 'text/plain')
        self.assertIn(raw, response.text)

        response = session.get(self.server_url + "/static/Test5.md?badParameter", timeout=5)
        print(response.text)
        self.assertEqual(response.status_code, 400)
        self.assertEqual(response.headers['Content-Type'], 'text/plain')
        self.assertIn(bad_request, response.text)

    def test_notfound(self):
        session = requests.Session()
        response = session.get(self.server_url + "/", timeout=5)
        print(response.text)
        expected_response = "<html><head><title>Not Found</title></head><body><h1>404 Not Found</h1></body></html>"

        self.assertEqual(response.status_code, 404)
        self.assertEqual(response.headers['Content-Type'], 'text/html')
        self.assertIn(expected_response, response.text)

    def test_crud(self):
        session = requests.Session()
        data = {"brand": "Nike", "model": "Air Max", "size": 10}

        response_post = session.post(self.server_url + "/api/Shoes", json=data, timeout=5)
        print(response_post.text)
        
        self.assertEqual(response_post.status_code, 201)
        self.assertEqual(response_post.headers['Content-Type'], 'application/json')

        print(response_post.json())
        res_id = response_post.json()["id"]

        response_get = session.get(self.server_url + "/api/Shoes/" + res_id, timeout=5)
        print(response_get.text)
        
        self.assertEqual(response_get.status_code, 200)
        self.assertEqual(response_get.headers['Content-Type'], 'application/json')
        self.assertEqual(response_get.json(), data)

        response_delete = session.delete(self.server_url + "/api/Shoes/" + res_id, timeout=5)
        print(response_delete.text)

        self.assertEqual(response_delete.status_code, 204)

        response_get_2 = session.get(self.server_url + "/api/Shoes/" + res_id, timeout=5)
        print(response_get_2.text)

        self.assertEqual(response_get_2.status_code, 404)

    def test_markdown(self):
        session = requests.Session()
        data = "Here is some **bold** text and some *italic* text."
        expected_response = "<p>Here is some <strong>bold</strong> text and some <em>italic</em> text.</p>\n"

        response_post = session.post(self.server_url + "/markdown/Shoes", data=data, headers={'Content-Type': 'text/markdown'}, timeout=5)
        print(response_post.text)
        
        self.assertEqual(response_post.status_code, 201)
        self.assertEqual(response_post.headers['Content-Type'], 'application/json')

        print(response_post.json())
        res_id = response_post.json()["id"]

        response_get_raw = session.get(self.server_url + "/markdown/Shoes/" + res_id + "?raw=true", timeout=5)
        print(response_get_raw.text)
        
        self.assertEqual(response_get_raw.status_code, 200)
        self.assertEqual(response_get_raw.headers['Content-Type'], 'text/markdown')
        self.assertEqual(response_get_raw.text, data)

        response_get = session.get(self.server_url + "/markdown/Shoes/" + res_id, timeout=5)
        print(response_get.text)
        
        self.assertEqual(response_get.status_code, 200)
        self.assertEqual(response_get.headers['Content-Type'], 'text/html')
        self.assertEqual(response_get.text, expected_response)

        response_delete = session.delete(self.server_url + "/markdown/Shoes/" + res_id, timeout=5)
        print(response_delete.text)

        self.assertEqual(response_delete.status_code, 204)

        response_get_2 = session.get(self.server_url + "/markdown/Shoes/" + res_id, timeout=5)
        print(response_get_2.text)

        self.assertEqual(response_get_2.status_code, 404)

    def test_multithreading(self):
        session = requests.Session()
        response = session.get(self.server_url + "/sleep", timeout=5)
        begin = time.time()
        response = session.get(self.server_url + "/echo", timeout=5)
        end = time.time()
        print(response.text)

        self.assertEqual(response.status_code, 200)
        self.assertLess(end - begin, 1)

    def test_health(self):
        session = requests.Session()
        response = session.get(self.server_url + "/health", timeout=5)
        print(response.text)
        expected_response = "OK"

        self.assertEqual(response.status_code, 200)
        self.assertEqual(response.headers['Content-Type'], 'text/plain')
        self.assertIn(expected_response, response.text)

    def test_large_request(self):
        data = {
            "text": "a" * 2000
        }

        session = requests.Session()
        response = session.get(self.server_url + "/echo", json=data, timeout=5)
        print(response.text)
        self.assertEqual(response.status_code, 200)


if __name__ == '__main__':
    unittest.main()
