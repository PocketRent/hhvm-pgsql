
drop table test;
create table test(
  id serial primary key,
  name text,
  valid boolean,
  someint integer,
  sometime timestamp
);

drop table complete;
create table complete(
  smallintex smallint,
  integerex integer,
  bigintex bigint,
  decimalex decimal,
  numericex numeric,
  realex real,
  doubleex double precision,
  smallserialex smallserial,
  serialex serial,
  bigserialex bigserial
);

insert into complete values(1,2,3,4,5,6.5,7.6,8,9,10);

