Переходим в дирректорию, где хранятся сертификат и ключ
```console
cd server
cd cert
```
Создаем с помощью Lets encrypt сертификат и ключ
```console
openssl req -new -x509 -days 365 -nodes -out server.crt -keyout server.key
```
Вводим данные -> проверяем.
Проверка закрытого ключа. Если все хорошо, выведет RSA key ok
```console
openssl rsa -in server.key -check
```
Для проверки сертификата. Эта команда выведет все данные о сертификате
```console
openssl x509 -in server.crt -text -noout
```
Сверяем сертификат и ключ. Должны быть одинаковые хэши
```console
openssl x509 -noout -modulus -in server.crt | openssl md5
openssl rsa -noout -modulus -in server.key | openssl md5
```
