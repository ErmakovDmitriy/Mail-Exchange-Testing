Данная программа работает по следующему принципу:
<br>1)Отправляет несколько тестовых сообщений на адрес SendeTestMessageTo, используя учётные данные указанные в SenderMailConfiguration
<br>2)Ожидает время указанное в DelieverTimeout
<br>3)Получает всю почту с адреса Receiver Mail Configuration
<br>4)Проверяет были ли доставлены все сообщения
<br>5) Если число ошибок доставки превышает threshhold, то с использованием учётных данных Error Mail Configuration на адрес SendErrorMessageTo отправляется уведомление об ошибке.

<br>Конфигурация считывается из файла config.cfg, должен находиться в папке с программой.

Для компиляции программы необходимо наличие установленных библиотек POCO. Исходные коды библиотек и инструкции по установке можно найти на http://pocoproject.org/
