package com.springboot.model;

import jakarta.persistence.*;
import java.time.LocalDateTime;

@Entity
@Table(name = "portfolio_snapshot", indexes =
{
    @Index(name = "idx_snapshot_time", columnList = "snapshot_time"),
    @Index(name = "idx_created_at", columnList = "created_at")
})

public class PortfolioSnapshot
{
    @Id
    @GeneratedValue(strategy = GenerationType.IDENTITY)
    private Long id;

    @Column(name = "snapshot_time", nullable = false, columnDefinition = "DATETIME(3)")
    private LocalDateTime snapshotTime;

    @Column(name = "total_value", nullable = false)
    private Long totalValue;

    @Column(name = "total_purchase_price")
    private Long totalPurchasePrice = 0L;

    @Column(name = "total_profit_loss")
    private Long totalProfitLoss = 0L;

    @Column(name = "profit_rate", precision = 10, scale = 4)
    private Double profitRate = 0.0;

    @Column(name = "stock_count")
    private Integer stockCount = 0;

    @Column(name = "created_at", nullable = false, updatable = false)
    private LocalDateTime createdAt;

    @PrePersist
    protected void onCreate()
    {
        createdAt = LocalDateTime.now();
        
        if (snapshotTime == null)
            snapshotTime = LocalDateTime.now();
    }

    // Constructors
    public PortfolioSnapshot() { }

    public PortfolioSnapshot(LocalDateTime snapshotTime, Long totalValue, Long totalPurchasePrice, 
                           Long totalProfitLoss, Double profitRate, Integer stockCount)
    {
        this.snapshotTime = snapshotTime;
        this.totalValue = totalValue;
        this.totalPurchasePrice = totalPurchasePrice;
        this.totalProfitLoss = totalProfitLoss;
        this.profitRate = profitRate;
        this.stockCount = stockCount;
    }

    // Getters and Setters
    public Long getId() { return id; }
    public void setId(Long id) { this.id = id; }

    public LocalDateTime getSnapshotTime() { return snapshotTime; }
    public void setSnapshotTime(LocalDateTime snapshotTime) { this.snapshotTime = snapshotTime; }

    public Long getTotalValue() { return totalValue; }
    public void setTotalValue(Long totalValue) { this.totalValue = totalValue; }

    public Long getTotalPurchasePrice() { return totalPurchasePrice; }
    public void setTotalPurchasePrice(Long totalPurchasePrice) { this.totalPurchasePrice = totalPurchasePrice; }

    public Long getTotalProfitLoss() { return totalProfitLoss; }
    public void setTotalProfitLoss(Long totalProfitLoss) { this.totalProfitLoss = totalProfitLoss; }

    public Double getProfitRate() { return profitRate; }
    public void setProfitRate(Double profitRate) { this.profitRate = profitRate; }

    public Integer getStockCount() { return stockCount; }
    public void setStockCount(Integer stockCount) { this.stockCount = stockCount; }

    public LocalDateTime getCreatedAt() { return createdAt; }
    public void setCreatedAt(LocalDateTime createdAt) { this.createdAt = createdAt; }

    @Override
    public String toString()
    {
        return "PortfolioSnapshot{" +
                "id=" + id +
                ", snapshotTime=" + snapshotTime +
                ", totalValue=" + totalValue +
                ", totalPurchasePrice=" + totalPurchasePrice +
                ", totalProfitLoss=" + totalProfitLoss +
                ", profitRate=" + profitRate +
                ", stockCount=" + stockCount +
                ", createdAt=" + createdAt +
                '}';
    }
}
