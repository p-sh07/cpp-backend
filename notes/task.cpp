Как передавать токен от клиента к серверу?

Авторизацию могут требовать не только запросы к API, но и запросы к обычным ресурсам на сервере. Например, пользователь социальной сети может ограничить доступ к фотографиям самому себе и близким друзьям. Прочие пользователи, даже зная URL фотографии, не должны получить к ней доступ. По этой причине URL-строка запроса не подходит для размещения токена. Не подходит и тело запроса, так как GET- и HEAD-запросы тела не имеют.
Для передачи информации, необходимой для аутентификации и авторизации, в протоколе HTTP служит заголовок Authorization. Чтобы передать токен, значение этого заголовка должно быть Bearer <токен>. Слово Bearer (англ. bearer — «предъявитель») говорит серверу: предоставь мне доступ к ресурсу по этому токену. Проверив подлинность токена и наличие у его предъявителя доступа к запрашиваемому ресурсу, сервер должен обработать запрос. При невалидном токене или отсутствующем заголовке Authorization — отклонить запрос с кодом 401 Unauthorized. Если токен валиден, но у предъявителя токена нет прав на доступ к ресурсу, сервер должен вернуть ответ 403 Forbidden.
Пример запроса с заголовком Authorization.
GET /api/v1/game/players HTTP/1.1
Authorization: Bearer very-secret-token-123
Host: mysite.com 
От теории перейдём к практике и реализуем вход в игру и получение информации об игроках.
Задание 1

В каталоге sprint2/problems/join_game/precode вы найдёте заготовку кода для этого задания: набор статических файлов и файл конфигурации. Своё решение разместите в каталоге sprint2/problems/join_game/solution.
Терминология
Пёс — игровой объект, способный перемещаться по игровой карте и взаимодействовать с другими объектами в соответствии с правилами игры.
Игровой сервер или просто сервер — программа, которая выполняется на серверном компьютере. Реализует игровую логику и предоставляет REST API.
Клиент — программное обеспечение, которое выполняется в браузере. Оно может взаимодействовать с сервером и визуализировать игровое состояние.
Пользователь — человек, участвующий в игре при помощи клиентского ПО.
Игрок — агент внутри игрового сервера, при помощи которого пользователь может управлять своим псом.
Токен — псевдослучайная последовательность символов, известная только пользователю и серверу. Поэтому пользователь может управлять лишь своим игроком, ведь токенов других игроков он не знает.
Реализуйте в игровом сервере операции входа в игру и получения информации об игроках. Пользователь может присоединиться к игре на любой из доступных карт. Для входа в игру он должен сообщить серверу кличку своего пса и id карты.
Сервер должен создать пса на указанной карте и игрока, управляющего псом, а затем вернуть клиенту id игрока и токен для аутентификации. Id нужен клиенту, чтобы отличать своего игрока от других. Клиент должен предъявить токен серверу, чтобы получать информацию о состоянии игрового сеанса и управлять своим игроком.
Вход в игру

Для входа в игру реализуйте обработку POST-запроса к точке входа /api/v1/game/join. Параметры запроса:
Обязательный заголовок Content-Type должен иметь тип application/json.
Тело запроса — JSON-объект с обязательными полями userName и mapId: имя игрока и id карты. Имя игрока совпадает с именем пса.
Пример запроса:
POST /api/v1/game/join HTTP/1.1
Content-Type: application/json

{"userName": "Scooby Doo", "mapId": "map1"} 
В случае успеха должен возвращаться ответ, обладающий следующими свойствами:
Статус-код 200 OK.
Заголовок Content-Type должен иметь тип application/json.
Заголовок Content-Length должен хранить размер тела ответа.
Обязательный заголовок Cache-Control должен иметь значение no-cache.
Тело ответа — JSON-объект с полями authToken и playerId. Поле playerId — целое число, задающее id игрока. Поле authToken — токен для авторизации в игре — строка, состоящая из 32 случайных шестнадцатеричных цифр.
Пример ответа:
HTTP/1.1 200 OK
Content-Type: application/json
Content-Length: 61
Cache-Control: no-cache

{"authToken":"6516861d89ebfff147bf2eb2b5153ae1","playerId":0} 
Если в качестве mapId указан несуществующий id карты, должен вернуться ответ со статус-кодом 404 Not found и заголовками Content-Length: <размер тела ответа>, Content-Type: application/json и Cache-Control: no-cache. Тело ответа — JSON-объект с полями code и message. Поле code — строка "mapNotFound". Поле message — строка с понятным человеку текстом ошибки. Пример:
HTTP/1.1 404 Not found
Content-Type: application/json
Content-Length: 51
Cache-Control: no-cache

{"code": "mapNotFound", "message": "Map not found"} 
Если было передано пустое имя игрока, должен вернуться ответ со статус-кодом 400 Bad request и заголовками Content-Length: <размер тела ответа>, Content-Type: application/json и Cache-Control: no-cache. Тело ответа — JSON-объект с полями code и message. Поле code — строка "invalidArgument". Поле message — строка с понятным человеку текстом ошибки. Пример:
HTTP/1.1 400 Bad request
Content-Type: application/json
Content-Length: 54
Cache-Control: no-cache

{"code": "invalidArgument", "message": "Invalid name"} 
Если при парсинге JSON или получении его свойств произошла ошибка, должен вернуться ответ со статус-кодом 400 Bad request и заголовками Content-Length: <размер тела ответа>, Content-Type: application/json и Cache-Control: no-cache. Тело ответа — JSON-объект с полями code и message. Поле code — строка "invalidArgument". Поле message — строка с понятным человеку текстом ошибки. Пример:
HTTP/1.1 400 Bad request
Content-Type: application/json
Content-Length: 71
Cache-Control: no-cache

{"code": "invalidArgument", "message": "Join game request parse error"} 
Если метод запроса отличается от POST, должен вернуться ответ со статус-кодом 405 Method Not Allowed и заголовками Content-Length: <размер тела ответа>, Content-Type: application/json, Allow: POST и Cache-Control: no-cache. Тело ответа — JSON-объект с полями code и message. Поле code — строка "invalidMethod", а message — строка с понятным человеку текстом ошибки. Пример:
HTTP/1.1 405 Method Not Allowed
Content-Type: application/json
Allow: POST
Content-Length: 68
Cache-Control: no-cache

{"code": "invalidMethod", "message": "Only POST method is expected"} 
Получение списка игроков

Чтобы получить список игроков, находящихся в одной игровой сессии с игроком, используется GET-запрос к точке входа /api/v1/game/players. Параметры запроса:
Обязательный заголовок Authorization: Bearer <токен пользователя>. В качестве токена пользователя следует передать токен, полученный при входе в игру. Этот токен сервер использует, чтобы аутентифицировать игрока и определить, на какой карте он находится.
Пример запроса:
GET /api/v1/game/players HTTP/1.1
Authorization: Bearer 6516861d89ebfff147bf2eb2b5153ae1  
В случае успеха должен вернуться ответ со следующими свойствами:
Статус-код 200 OK.
Заголовок Content-Type: application/json.
Заголовок Content-Length: <размер тела ответа>.
Заголовок Cache-Control: no-cache.
Тело ответа — JSON-объект. Его ключи — идентификаторы пользователей на карте. Значение каждого из этих ключей — JSON-объект с единственным полем name, строкой, задающей имя пользователя, под которым он вошёл в игру.
Пример ответа:
HTTP/1.1 200 OK
Content-Type: application/json
Content-Length: 75
Cache-Control: no-cache

{
  "0": {"name": "Harry Potter"},
  "1": {"name": "Hermione Granger"}
} 
Если заголовок Authorization в запросе отсутствует либо его значение не удовлетворяет требованиям задачи, должен вернуться ответ со следующими свойствами:
Статус-код 401 Unauthorized.
Заголовок Content-Type: appication/json.
Заголовок Content-Length: <размер тела ответа>.
Заголовок Cache-Control: no-cache.
Тело ответа — JSON-объект с полями code и message. Поле code должно иметь значение "invalidToken", а message — содержать понятное человеку описание ошибки.
Пример:
HTTP/1.1 401 Unauthorized
Content-Type: application/json
Content-Length: 70
Cache-Control: no-cache

{"code": "invalidToken", "message": "Authorization header is missing"} 
Если заголовок Authorization содержит валидное значение токена, но в игре нет пользователя с таким токеном, должен вернуться ответ со следующими свойствами:
Статус-код 401 Unauthorized.
Заголовок Content-Type: appication/json.
Заголовок Content-Length: <размер тела ответа>.
Заголовок Cache-Control: no-cache.
Тело ответа — JSON-объект с полями code и message.
Пример:
HTTP/1.1 401 Unauthorized
Content-Type: application/json
Content-Length: 70
Cache-Control: no-cache

{"code": "unknownToken", "message": "Player token has not been found"} 
Если метод запроса отличается от GET или HEAD, должен вернуться ответ со следующими свойствами:
Статус-код 405 Method Not Allowed.
Заголовок Content-Type: appication/json.
Заголовок Allow: GET, HEAD.
Заголовок Content-Length: <размер тела ответа>.
Заголовок Cache-Control: no-cache.
Тело ответа — JSON-объект с полями code и message.
Пример:
HTTP/1.1 405 Method Not Allowed
Content-Type: application/json
Allow: GET, HEAD
Content-Length: 54
Cache-Control: no-cache

{"code": "invalidMethod", "message": "Invalid method"} 
Как будет тестироваться ваш код

В репозитории будут тесты, которые запустят ваш сервер, передав ему путь к конфигурационному файлу и путь к каталогу со статическими файлами. Тесты проверят, правильно ли сервер обрабатывает запросы к REST API и отдаёт статические файлы.
Рекомендации

Если класс ResponseHandler (или его аналог в вашей программе) обрабатывает как запросы статических файлов, так и запросы к API, то сейчас самое время вынести обработку API запросов в отдельный класс. Ваш сервер будет обрабатывать всё больше и больше запросов, и ResponseHandler станет сложнее поддерживать.
Начиная с этого задания, ваш сервер будет не только возвращать состояние игры, но и изменять его. Бизнес-правила игры не должны зависеть от того, по какому протоколу происходит общение клиента с сервером. Это значит, что код, отвечающий за игровую логику, стоит отделить от кода, занимающегося обработкой HTTP-запросов. Так вы сделаете свой код проще.
Прежде чем писать код, рекомендуем спроектировать архитектуру. Править схему легче, чем рефакторить уже написанный код. Обязательно сохраните нарисованные схемы — они вам пригодятся в будущих заданиях.