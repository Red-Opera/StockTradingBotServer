package com.springboot.service;

import com.springboot.data.HoldingSnapshotRepository;
import com.springboot.data.PortfolioSnapshotRepository;
import com.springboot.model.Holding;
import com.springboot.model.HoldingSnapshot;
import com.springboot.model.PortfolioSnapshot;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;

import java.time.LocalDateTime;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;

@Service
public class SnapshotService {
    private static final Logger log = LoggerFactory.getLogger(SnapshotService.class);

    private final PortfolioSnapshotRepository portfolioSnapshotRepository;
    private final HoldingSnapshotRepository holdingSnapshotRepository;

    private LocalDateTime lastSnapshotTime = null;
    private static final long SNAPSHOT_INTERVAL_MS = 5000; // 스냅샷 저장 간격 (5초)

    @Autowired
    public SnapshotService(PortfolioSnapshotRepository portfolioSnapshotRepository,
            HoldingSnapshotRepository holdingSnapshotRepository) {
        this.portfolioSnapshotRepository = portfolioSnapshotRepository;
        this.holdingSnapshotRepository = holdingSnapshotRepository;
    }

    /**
     * 주기적으로 스냅샷을 데이터베이스에 저장하는 메서드.
     * 외부 빈에서 호출되므로 @Transactional이 Spring 프록시를 통해 정상 동작합니다.
     */
    @Transactional
    public void SaveSnapshotIfNeeded(Map<String, Holding> latestHoldings)
    {
        try
        {
            LocalDateTime now = LocalDateTime.now();

            // 마지막 저장 시간 체크 (너무 자주 저장하지 않도록)
            if (lastSnapshotTime != null) 
            {
                long elapsedMs = java.time.Duration.between(lastSnapshotTime, now).toMillis();

                // 아직 간격이 충분하지 않음
                if (elapsedMs < SNAPSHOT_INTERVAL_MS)
                    return; 
            }

            // 저장할 데이터가 없음
            if (latestHoldings.isEmpty())
                return;

            // 개별 종목 스냅샷 저장
            List<HoldingSnapshot> holdingSnapshots = new ArrayList<>();
            long totalValue = 0;
            long totalPurchasePrice = 0;
            long totalProfitLoss = 0;
            int stockCount = 0;

            for (Holding holding : latestHoldings.values())
            {
                HoldingSnapshot snapshot = HoldingSnapshot.fromHolding(holding);
                snapshot.setSnapshotTime(now);
                holdingSnapshots.add(snapshot);

                totalValue += holding.getValue();
                totalPurchasePrice += holding.getPurchasePrice();
                totalProfitLoss += holding.getProfitLoss();
                stockCount++;
            }

            // 배치로 저장
            holdingSnapshotRepository.saveAll(holdingSnapshots);

            // 포트폴리오 전체 스냅샷 저장
            double profitRate = totalPurchasePrice > 0 ? ((double) totalProfitLoss / totalPurchasePrice) * 100.0 : 0.0;

            PortfolioSnapshot portfolioSnapshot = new PortfolioSnapshot(
                    now,
                    totalValue,
                    totalPurchasePrice,
                    totalProfitLoss,
                    profitRate,
                    stockCount);

            portfolioSnapshotRepository.save(portfolioSnapshot);

            lastSnapshotTime = now;

            log.info("Saved snapshot: {} holdings, total value: {}", stockCount, totalValue);
        } 
        
        catch (Exception e)
        {
            log.error("Failed to save snapshot to database", e);
        }
    }
}
