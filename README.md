Данная программа работает по следующему принципу:
1)Отправляет несколько тестовых сообщений на адрес SendeTestMessageTo, используя учётные данные указанные в SenderMailConfiguration
2)Ожидает время указанное в DelieverTimeout
3)Получает всю почту с адреса Receiver Mail Configuration
4)Проверяет были ли доставлены все сообщения
5) Если число ошибок доставки превышает threshhold, то с использованием учётных данных Error Mail Configuration на адрес SendErrorMessageTo отправляется уведомление об ошибке.

Конфигурация считывается из файла config.cfg, должен находиться в папке с программой.