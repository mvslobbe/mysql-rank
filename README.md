# mysql-rank
Custom 'RANK' User Defined Function for Mysql.

# installation
Run 'Makefile'. This works well on Ubuntu systems.<BR>
In mysql, run 'CREATE FUNCTION rank RETURNS INT SONAME 'udf_rank.so';'<BR>

# sample usage
Consider a datatabase:<BR>
INSERT INTO FOO(integer) VALUES(1);<BR>
INSERT INTO FOO(integer) VALUES(1);<BR>
INSERT INTO FOO(integer) VALUES(2);<BR>
<BR>
And the query:<BR>
SELECT integer, RANK(integer) FROM FOO -><BR>
1 0 // 0 successive '1' ; counter starts at 0<BR>
1 1 // 1 successive '1' ; counter gets incremented because 1 == 1<BR>
2 0 // 0 successive '2' ; counter gets reset because 2 != 1<BR>

