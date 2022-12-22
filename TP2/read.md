# RCOM Setup

>Num tux qualquer(4) ligar a S0 à consola do switch, fazer login no admin e correr:

`/system reset-configuration`

>Trocar da consola do switch para a consola MTIK



### Ligar os cabos:

![](https://i.imgur.com/usIhgWv.jpg)

![](https://i.imgur.com/9GQUAOz.jpg)


### Configurar as portas do switch:
- no tux2 : `ifconfig eth0 172.16.31.1/24`
- no tux3 : `ifconfig eth0 172.16.30.1/24`
- no tux4 : 
    - `ifconfig eth0 172.16.30.254/24`
    - `ifconfig eth1 172.16.31.253/24`
- no Rc(no terminal no tux 4 onde é feita a ligação ao router MTIK):
    - `/ip address add address=172.16.2.39/24 interface=ether1`
    - `/ip address add address=172.16.31.254/24 interface=ether2`

>Trocar da consola MTIK para a consola do switch 

### Criar as bridges:
- `/interface bridge add name=bridge30`
- `/interface bridge add name=bridge31`
>Para verificar: `/interface bridge print`

>Para ver as portas: `/interface bridge port print`

### Remover as portas ligadas à default bridge:
- `/interface bridge port remove [find interface =ether2]`
- `/interface bridge port remove [find interface =ether3]`
- `/interface bridge port remove [find interface =ether6]`
- `/interface bridge port remove [find interface =ether10]`
- `/interface bridge port remove [find interface =ether14]`

>Verificar a remoção das portas

### Adicionar as portas às bridges 30 e 31:
- `/interface bridge port add bridge=bridge30 interface=ether10`
- `/interface bridge port add bridge=bridge30 interface=ether14`
- `/interface bridge port add bridge=bridge31 interface=ether2`
- `/interface bridge port add bridge=bridge31 interface=ether3`
- `/interface bridge port add bridge=bridge31 interface=ether6`

>Verificar a inclusão das portas

### No terminal do tux4:

Enable IP forwarding
`echo 1 > /proc/sys/net/ipv4/ip_forward`
Disable ICMP echo ignore broadcast
`echo 0 > /proc/sys/net/ipv4/icmp_echo_ignore_broadcasts`

>Trocar da consola do switch para a consola MTIK

### Adicionar as gateways:
- no tux2:
    - `route add -net 172.16.30.0/24 gw 172.16.31.253`
    - `route add default gw 172.16.31.254`
- no tux3:
    - `route add -net 172.16.31.0/24 gw 172.16.30.254` (acho que não é preciso fazer)
    - `route add default gw 172.16.30.254`
- no tux4:
    - `route add default gw 172.16.31.254`
- no Rc(no terminal no tux 4 onde é feita a ligação ao router MTIK):
    - `/ip route add dst-address=172.16.30.0/24 gateway=172.16.31.253`
    - `/ip route add dst-address=0.0.0.0/0 gateway=172.16.2.254`

>Continuar para o passo 4 da exp 4

- no tux2:
    - `echo 0 > /proc/sys/net/ipv4/conf/eth0/accept_redirects`
    - `echo 0 > /proc/sys/net/ipv4/conf/all/accept_redirects`
    - `route del -net 172.16.30.0/24`

Depois de eliminar a route para o tux3 e dar enable aos redirects, o ping feito ao tux3 continua a lá chegar, pois o Rc tem uma rota para o tux3! No wireshark são visíveis os redirects.

> Passo 5, 6, e 7

No tux3, pingar a rede rc. Com nat ele chega lá.
 - `/ip firewall nat disable 0`
Agora a resposta é destination unreachable