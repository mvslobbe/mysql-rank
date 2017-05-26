# mysql-rank
Custom 'RANK' User Defined Function for Mysql.

# installation
Run 'Makefile'. This works well on Ubuntu systems.

# sample usage
Consider a datatabase:
INSERT INTO FOO(integer) VALUES(1);
INSERT INTO FOO(integer) VALUES(1);
INSERT INTO FOO(integer) VALUES(2);

And the query:
SELECT integer, RANK(integer) FROM FOO ->
1 0 // 0 successive '1' ; counter starts at 0
1 1 // 1 successive '1' ; counter gets incremented because 1 == 1
2 1 // 0 successive '2' ; counter gets reset because 2 != 1
