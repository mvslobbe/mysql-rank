# mysql-rank
Custom 'RANK' User Defined Function for Mysql.

# installation
Run 'Makefile'. This works well on Ubuntu systems.

# sample usage
Consider a datatabase:
INSERT INTO FOO(integer) VALUES(1);<br>
INSERT INTO FOO(integer) VALUES(1);<br>
INSERT INTO FOO(integer) VALUES(2);<br>

And the query:
SELECT integer, RANK(integer) FROM FOO -><br>
1 0 // 0 successive '1' ; counter starts at 0<br>
1 1 // 1 successive '1' ; counter gets incremented because 1 == 1<br>
2 1 // 0 successive '2' ; counter gets reset because 2 != 1<br>

