package com.springboot.model;

import jakarta.persistence.*;
import java.time.LocalDateTime;

@Entity
@Table(name = "holding_snapshot", indexes = {
        @Index(name = "idx_snapshot_time", columnList = "snapshot_time"),
        @Index(name = "idx_code", columnList = "code"),
        @Index(name = "idx_snapshot_code", columnList = "snapshot_time, code")
})
public class HoldingSnapshot {
    @Id
    @GeneratedValue(strategy = GenerationType.IDENTITY)
    private Long id;

    @Column(name = "snapshot_time", nullable = false, columnDefinition = "DATETIME(3)")
    private LocalDateTime snapshotTime;

    @Column(name = "account", length = 50)
    private String account;

    @Column(name = "code", nullable = false, length = 20)
    private String code;

    @Column(name = "name", length = 100)
    private String name;

    @Column(name = "quantity")
    private Long quantity = 0L;

    @Column(name = "price")
    private Long price = 0L;

    @Column(name = "value")
    private Long value = 0L;

    @Column(name = "purchase_price")
    private Long purchasePrice = 0L;

    @Column(name = "profit_loss")
    private Long profitLoss = 0L;

    @Column(name = "profit_rate")
    private Double profitRate = 0.0;

    @Column(name = "prev_close_price")
    private Long prevClosePrice = 0L;

    @Column(name = "daily_profit_rate")
    private Double dailyProfitRate = 0.0;

    @Column(name = "created_at", nullable = false, updatable = false)
    private LocalDateTime createdAt;

    @PrePersist
    protected void onCreate() {
        createdAt = LocalDateTime.now();

        if (snapshotTime == null) {
            snapshotTime = LocalDateTime.now();
        }
    }

    // Constructors
    public HoldingSnapshot() {
    }

    public HoldingSnapshot(LocalDateTime snapshotTime, String account, String code, String name,
            Long quantity, Long price, Long value, Long purchasePrice,
            Long profitLoss, Double profitRate, Long prevClosePrice, Double dailyProfitRate) {
        this.snapshotTime = snapshotTime;
        this.account = account;
        this.code = code;
        this.name = name;
        this.quantity = quantity;
        this.price = price;
        this.value = value;
        this.purchasePrice = purchasePrice;
        this.profitLoss = profitLoss;
        this.profitRate = profitRate;
        this.prevClosePrice = prevClosePrice;
        this.dailyProfitRate = dailyProfitRate;
    }

    // Factory method to create from Holding
    public static HoldingSnapshot fromHolding(Holding holding) {
        return new HoldingSnapshot(
                LocalDateTime.now(),
                holding.getAccount(),
                holding.getCode(),
                holding.getName(),
                holding.getQuantity(),
                holding.getPrice(),
                holding.getValue(),
                holding.getPurchasePrice(),
                holding.getProfitLoss(),
                holding.getProfitRate(),
                holding.getPrevClosePrice(),
                holding.getDailyProfitRate());
    }

    // Getters and Setters
    public Long getId() {
        return id;
    }

    public void setId(Long id) {
        this.id = id;
    }

    public LocalDateTime getSnapshotTime() {
        return snapshotTime;
    }

    public void setSnapshotTime(LocalDateTime snapshotTime) {
        this.snapshotTime = snapshotTime;
    }

    public String getAccount() {
        return account;
    }

    public void setAccount(String account) {
        this.account = account;
    }

    public String getCode() {
        return code;
    }

    public void setCode(String code) {
        this.code = code;
    }

    public String getName() {
        return name;
    }

    public void setName(String name) {
        this.name = name;
    }

    public Long getQuantity() {
        return quantity;
    }

    public void setQuantity(Long quantity) {
        this.quantity = quantity;
    }

    public Long getPrice() {
        return price;
    }

    public void setPrice(Long price) {
        this.price = price;
    }

    public Long getValue() {
        return value;
    }

    public void setValue(Long value) {
        this.value = value;
    }

    public Long getPurchasePrice() {
        return purchasePrice;
    }

    public void setPurchasePrice(Long purchasePrice) {
        this.purchasePrice = purchasePrice;
    }

    public Long getProfitLoss() {
        return profitLoss;
    }

    public void setProfitLoss(Long profitLoss) {
        this.profitLoss = profitLoss;
    }

    public Double getProfitRate() {
        return profitRate;
    }

    public void setProfitRate(Double profitRate) {
        this.profitRate = profitRate;
    }

    public Long getPrevClosePrice() {
        return prevClosePrice;
    }

    public void setPrevClosePrice(Long prevClosePrice) {
        this.prevClosePrice = prevClosePrice;
    }

    public Double getDailyProfitRate() {
        return dailyProfitRate;
    }

    public void setDailyProfitRate(Double dailyProfitRate) {
        this.dailyProfitRate = dailyProfitRate;
    }

    public LocalDateTime getCreatedAt() {
        return createdAt;
    }

    public void setCreatedAt(LocalDateTime createdAt) {
        this.createdAt = createdAt;
    }

    @Override
    public String toString() {
        return "HoldingSnapshot{" +
                "id=" + id +
                ", snapshotTime=" + snapshotTime +
                ", account='" + account + '\'' +
                ", code='" + code + '\'' +
                ", name='" + name + '\'' +
                ", quantity=" + quantity +
                ", price=" + price +
                ", value=" + value +
                ", purchasePrice=" + purchasePrice +
                ", profitLoss=" + profitLoss +
                ", profitRate=" + profitRate +
                ", createdAt=" + createdAt +
                '}';
    }
}
