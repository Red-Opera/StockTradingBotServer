package com.springboot.model;

public class Holding
{
    private String account;
    private String code;
    private String name;

    private long quantity = 0;
    private long price = 0;
    private long value = 0;
    private long purchasePrice = 0;
    private long profitLoss = 0;
    private double profitRate = 0.0;

    public Holding() { }

    public String getAccount() { return account; }
    public String getCode() { return code; }
    public String getName() { return name; }
    public long getQuantity() { return quantity; }
    public long getPrice() { return price; }
    public long getValue() { return value; }
    public long getPurchasePrice() { return purchasePrice; }
    public long getProfitLoss() { return profitLoss; }
    public double getProfitRate() { return profitRate; }

    public void setAccount(String account) { this.account = account; }
    public void setCode(String code) { this.code = code; }
    public void setName(String name) { this.name = name; }
    public void setQuantity(long quantity) { this.quantity = quantity; }
    public void setPrice(long price) { this.price = price; }
    public void setValue(long value) { this.value = value; }
    public void setPurchasePrice(long purchasePrice) { this.purchasePrice = purchasePrice; }
    public void setProfitLoss(long profitLoss) { this.profitLoss = profitLoss; }
    public void setProfitRate(double profitRate) { this.profitRate = profitRate; }

    @Override
    public String toString() {
        return "Holding{" +
                "account='" + account + '\'' +
                ", code='" + code + '\'' +
                ", name='" + name + '\'' +
                ", quantity=" + quantity +
                ", price=" + price +
                ", value=" + value +
                ", purchasePrice=" + purchasePrice +
                ", profitLoss=" + profitLoss +
                ", profitRate=" + profitRate +
                '}';
    }
}
